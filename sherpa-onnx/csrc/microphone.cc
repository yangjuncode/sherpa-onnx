// sherpa-onnx/csrc/microphone.cc
//
// Copyright (c)  2022-2023  Xiaomi Corporation

#include "sherpa-onnx/csrc/microphone.h"

#include <stdio.h>
#include <stdlib.h>
#if !defined(_WIN32)
#include <unistd.h>
#include <fcntl.h>
#endif

namespace {
#if !defined(_WIN32)
// ALSA error handler type
using SndLibErrorHandler = void (*)(const char *, int, const char *, int,
                                    const char *, ...);

extern "C" {
// Declare ALSA's handler setter as a weak symbol so we don't need to link libasound.
// If ALSA is not present, the symbol will be null and skipped.
void snd_lib_error_set_handler(SndLibErrorHandler) __attribute__((weak));
}

// No-op handler to mute ALSA internal logs
static void AlsaSilentErrorHandler(const char *, int, const char *, int,
                                   const char *, ...) {}

// Try to silence ALSA messages at runtime without requiring libasound at link time
static void SuppressAlsaErrorsAtRuntime() {
  if (snd_lib_error_set_handler) {
    snd_lib_error_set_handler(AlsaSilentErrorHandler);
  }
}

// RAII silencer that temporarily redirects stderr to /dev/null
struct StderrSilencer {
  int saved_fd{-1};
  int null_fd{-1};
  StderrSilencer() {
    saved_fd = dup(STDERR_FILENO);
    null_fd = open("/dev/null", O_WRONLY);
    if (null_fd >= 0) {
      dup2(null_fd, STDERR_FILENO);
    }
  }
  ~StderrSilencer() {
    if (saved_fd >= 0) {
      dup2(saved_fd, STDERR_FILENO);
      close(saved_fd);
    }
    if (null_fd >= 0) {
      close(null_fd);
    }
  }
};
#endif
}  // namespace

namespace sherpa_onnx {

Microphone::Microphone() {
  PaError err;
#if !defined(_WIN32)
  // Mute ALSA errors and any stderr noise during PortAudio initialization
  SuppressAlsaErrorsAtRuntime();
  {
    StderrSilencer _silence_stderr;
    err = Pa_Initialize();
  }
#else
  err = Pa_Initialize();
#endif
  if (err != paNoError) {
    fprintf(stderr, "portaudio error: %s\n", Pa_GetErrorText(err));
    exit(-1);
  }
}

Microphone::~Microphone() {
  CloseDevice();
  PaError err = Pa_Terminate();
  if (err != paNoError) {
    fprintf(stderr, "portaudio error: %s\n", Pa_GetErrorText(err));
  }
}

int Microphone::GetDeviceCount() const { return Pa_GetDeviceCount(); }

int Microphone::GetDefaultInputDevice() const {
  return Pa_GetDefaultInputDevice();
}

void Microphone::PrintDevices(int device_index) const {
  int num_devices = Pa_GetDeviceCount();
  fprintf(stderr, "Num devices: %d\n", num_devices);
  for (int i = 0; i != num_devices; ++i) {
    const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
    fprintf(stderr, " %s %d %s\n", (i == device_index) ? "*" : " ", i,
            info->name);
  }
}

bool Microphone::OpenDevice(int index, int sample_rate, int channel,
                            PaStreamCallback cb, void *userdata) {
  if (index < 0 || index >= Pa_GetDeviceCount()) {
    fprintf(stderr, "Invalid device index: %d\n", index);
    return false;
  }

  const PaDeviceInfo *info = Pa_GetDeviceInfo(index);
  if (!info) {
    fprintf(stderr, "No device info found for index: %d\n", index);
    return false;
  }

  CloseDevice();

  fprintf(stderr, "Use device: %d\n", index);
  fprintf(stderr, "  Name: %s\n", info->name);
  fprintf(stderr, "  Max input channels: %d\n", info->maxInputChannels);

  PaStreamParameters param;
  param.device = index;
  param.channelCount = channel;
  param.sampleFormat = paFloat32;
  param.suggestedLatency = info->defaultLowInputLatency;
  param.hostApiSpecificStreamInfo = nullptr;

  PaError err;
#if !defined(_WIN32)
  {
    // Some backends (e.g., JACK) may print to stderr when opening the stream
    StderrSilencer _silence_stderr;
    err = Pa_OpenStream(&stream, &param, nullptr, /* &outputParameters, */
                        sample_rate,
                        0,          // frames per buffer
                        paClipOff,  // we won't output out of range samples
                                    // so don't bother clipping them
                        cb, userdata);
  }
#else
  err = Pa_OpenStream(&stream, &param, nullptr, /* &outputParameters, */
                      sample_rate,
                      0,          // frames per buffer
                      paClipOff,  // we won't output out of range samples
                                  // so don't bother clipping them
                      cb, userdata);
#endif
  if (err != paNoError) {
    fprintf(stderr, "portaudio error: %s\n", Pa_GetErrorText(err));
    return false;
  }

#if !defined(_WIN32)
  {
    // Be safe and silence any startup noise from host APIs on stream start
    StderrSilencer _silence_stderr;
    err = Pa_StartStream(stream);
  }
#else
  err = Pa_StartStream(stream);
#endif
  fprintf(stderr, "Started\n");

  if (err != paNoError) {
    fprintf(stderr, "portaudio error: %s\n", Pa_GetErrorText(err));
    CloseDevice();
    return false;
  }
  return true;
}

void Microphone::CloseDevice() {
  if (stream) {
    PaError err = Pa_CloseStream(stream);
    if (err != paNoError) {
      fprintf(stderr, "Pa_CloseStream error: %s\n", Pa_GetErrorText(err));
    }
    stream = nullptr;
  }
}

}  // namespace sherpa_onnx
