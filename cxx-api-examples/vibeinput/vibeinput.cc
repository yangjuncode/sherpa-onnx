#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <iostream>
#include <mutex>  // NOLINT
#include <queue>
#include <vector>

#include <cstring>
#include <string.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <thread>
#include <atomic>

#include "typestr.h"

#include "portaudio.h"       // NOLINT
// #include "sherpa-display.h"  // NOLINT
#include "sherpa-onnx/c-api/cxx-api.h"
#include "sherpa-onnx/csrc/microphone.h"
#include "vibeinput.h"

std::queue<std::vector<float>> samples_queue;
std::condition_variable condition_variable;
std::mutex mutex;
static std::atomic<bool> g_stop{false};
static std::atomic<bool> g_paused{false};
static std::atomic<bool> g_running{false};
static std::thread g_worker;

// No SIGINT handler for GUI; stopping is controlled via API

static void TogglePause(const char *reason) {
  bool now = g_paused.load();
  g_paused.store(!now);
  std::cout << "[PAUSE] " << (now ? "Paused" : "Resumed")
      << (reason ? std::string(" by ") + reason : std::string())
      << "\n";
}

static bool HandleVoiceCommand(const std::string &text) {
  // Return true if the text is a pause/resume command and should NOT be typed
  // Commands (Chinese):
  //   Pause:  "停止语音输入" / "停止输入"
  //   Resume: "开启语音输入" / "启动语音输入"
  if (text.find("停止语音输入") != std::string::npos ||
      text.find("停止输入") != std::string::npos) {
    if (!g_paused.load()) {
      g_paused.store(true);
      sync_display();
      std::cout << "[PAUSE] Paused by voice command\n";
    }
    return true;
  }
  if (text.find("开启语音输入") != std::string::npos ||
      text.find("启动语音输入") != std::string::npos) {
    if (g_paused.load()) {
      g_paused.store(false);
      sync_display();
      std::cout << "[PAUSE] Resumed by voice command\n";
    }
    return true;
  }
  return false;
}

static int32_t RecordCallback(const void *input_buffer,
                              void * /*output_buffer*/,
                              unsigned long frames_per_buffer, // NOLINT
                              const PaStreamCallbackTimeInfo * /*time_info*/,
                              PaStreamCallbackFlags /*status_flags*/,
                              void * /*user_data*/) {
  std::lock_guard<std::mutex> lock(mutex);
  samples_queue.emplace(
      reinterpret_cast<const float *>(input_buffer),
      reinterpret_cast<const float *>(input_buffer) + frames_per_buffer);
  condition_variable.notify_one();

  return g_stop.load() ? paComplete : paContinue;
}

// ---------- Path utilities and CLI parsing ----------
static std::string GetHomeDir() {
  if (const char *home = std::getenv("HOME")) return std::string(home);
  if (const char *userprofile = std::getenv("USERPROFILE"))
    return std::string(userprofile);
  const char *homedrive = std::getenv("HOMEDRIVE");
  const char *homepath = std::getenv("HOMEPATH");
  if (homedrive && homepath)
    return
        std::string(homedrive) + std::string(homepath);
  return std::string();
}

static std::string ExpandUser(const std::string &path) {
  if (!path.empty() && path[0] == '~') {
    std::string home = GetHomeDir();
    if (!home.empty()) {
      if (path.size() == 1) return home;
      if (path.size() > 1 && (path[1] == '/' || path[1] == '\\')) {
        return (std::filesystem::path(home) / path.substr(2)).string();
      }
    }
  }
  return path;
}

static bool FileExists(const std::filesystem::path &p) {
  std::error_code ec;
  return std::filesystem::exists(p, ec) &&
         std::filesystem::is_regular_file(p, ec);
}

static std::string ResolveModelFile(const std::string &input_name) {
  namespace fs = std::filesystem;
  // 1) If input is an existing path (after ~ expansion), use it
  fs::path p = ExpandUser(input_name);
  if (FileExists(p)) return p.string();

  // 2) If user supplied an absolute or relative path but it doesn't exist, fail
  if (p.is_absolute() || p.has_parent_path()) {
    return std::string();
  }

  // 3) Search ./model-dir/<filename>
  fs::path p1 = fs::current_path() / "model-dir" / input_name;
  if (FileExists(p1)) return p1.string();

  // 4) Search ~/model-dir/<filename>
  std::string home = GetHomeDir();
  if (!home.empty()) {
    fs::path p2 = fs::path(home) / "model-dir" / input_name;
    if (FileExists(p2)) return p2.string();
  }

  return std::string();
}

static void ClearScreen() {
#ifdef _MSC_VER
  auto ret = system("cls");
#else
  auto ret = system("clear");
#endif
  (void)ret;
}

bool VibeInputIsPaused() {
  return g_paused.load();
}

static void PrintUsage(const char *prog) {
  std::cout << "Usage: " << prog
      << " [--vad-model filename] [--asr-model filename] [--tokens filename]\n"
      << "Search order for filenames: ./model-dir, ~/model-dir, or an explicit path.\n"
      << "Defaults: --vad-model silero_vad.onnx --asr-model model.int8.onnx --tokens tokens.txt\n";
}

static sherpa_onnx::cxx::VoiceActivityDetector CreateVad(
    const std::string &vad_model_path) {
  using namespace sherpa_onnx::cxx; // NOLINT
  VadModelConfig config;
  config.silero_vad.model = vad_model_path;
  config.silero_vad.threshold = 0.5;
  config.silero_vad.min_silence_duration = 0.1;
  config.silero_vad.min_speech_duration = 0.25;
  config.silero_vad.max_speech_duration = 8;
  config.sample_rate = 16000;
  config.debug = false;

  VoiceActivityDetector vad = VoiceActivityDetector::Create(config, 20);
  if (!vad.Get()) {
    std::cerr << "Failed to create VAD. Please check your config\n";
    exit(-1);
  }

  return vad;
}

static sherpa_onnx::cxx::OfflineRecognizer CreateOfflineRecognizer(
    const std::string &asr_model_path, const std::string &tokens_path) {
  using namespace sherpa_onnx::cxx; // NOLINT
  OfflineRecognizerConfig config;

  config.model_config.sense_voice.model = asr_model_path;
  config.model_config.sense_voice.use_itn = true;
  config.model_config.sense_voice.language = "auto";
  config.model_config.tokens = tokens_path;

  config.model_config.num_threads = 2;
  config.model_config.debug = false;

  std::cout << "Loading model\n";
  OfflineRecognizer recognizer = OfflineRecognizer::Create(config);
  if (!recognizer.Get()) {
    std::cerr << "Please check your config\n";
    exit(-1);
  }
  std::cout << "Loading model done\n";
  return recognizer;
}

// ---------- Worker main ----------
static int32_t WorkerMain(const VibeInputOptions &opts) {
  using namespace sherpa_onnx::cxx; // NOLINT

  // Resolve model paths
  std::string vad_model_name = opts.vad_model.empty() ? std::string("silero_vad.int8.onnx") : opts.vad_model;
  std::string asr_model_name = opts.asr_model.empty() ? std::string("model.int8.onnx") : opts.asr_model;
  std::string tokens_name    = opts.tokens.empty()    ? std::string("tokens.txt")            : opts.tokens;

  std::string vad_model_path = ResolveModelFile(vad_model_name);
  if (vad_model_path.empty()) {
    std::cerr << "Cannot find VAD model '" << vad_model_name
              << "' in ./model-dir or ~/model-dir, nor as a valid path.\n";
    return -1;
  }
  std::string asr_model_path = ResolveModelFile(asr_model_name);
  if (asr_model_path.empty()) {
    std::cerr << "Cannot find ASR model '" << asr_model_name
              << "' in ./model-dir or ~/model-dir, nor as a valid path.\n";
    return -1;
  }
  std::string tokens_path = ResolveModelFile(tokens_name);
  if (tokens_path.empty()) {
    std::cerr << "Cannot find tokens file '" << tokens_name
              << "' in ./model-dir or ~/model-dir, nor as a valid path.\n";
    return -1;
  }

  std::cout << "Using models:\n"
            << "  VAD:    " << vad_model_path << "\n"
            << "  ASR:    " << asr_model_path << "\n"
            << "  Tokens: " << tokens_path << "\n";

  auto vad = CreateVad(vad_model_path);
  auto recognizer = CreateOfflineRecognizer(asr_model_path, tokens_path);

  sherpa_onnx::Microphone mic;

  PaDeviceIndex num_devices = Pa_GetDeviceCount();
  if (num_devices == 0) {
    std::cerr << "  If you are using Linux, please try "
                 "./build/bin/sense-voice-simulate-streaming-alsa-cxx-api\n";
    return -1;
  }

  int32_t device_index = Pa_GetDefaultInputDevice();
  const char *pDeviceIndex = std::getenv("SHERPA_ONNX_MIC_DEVICE");
  if (pDeviceIndex) {
    fprintf(stderr, "Use specified device: %s\n", pDeviceIndex);
    device_index = atoi(pDeviceIndex);
  }
  mic.PrintDevices(device_index);

  float mic_sample_rate = 16000;
  const char *sample_rate_str = std::getenv("SHERPA_ONNX_MIC_SAMPLE_RATE");
  if (sample_rate_str) {
    fprintf(stderr, "Use sample rate %f for mic\n", mic_sample_rate);
    mic_sample_rate = atof(sample_rate_str);
  }
  float sample_rate = 16000;
  LinearResampler resampler;
  if (mic_sample_rate != sample_rate) {
    float min_freq = std::min(mic_sample_rate, sample_rate);
    float lowpass_cutoff = 0.99 * 0.5 * min_freq;

    int32_t lowpass_filter_width = 6;
    resampler = LinearResampler::Create(mic_sample_rate, sample_rate,
                                        lowpass_cutoff, lowpass_filter_width);
  }
  if (!mic.OpenDevice(device_index, mic_sample_rate, 1, RecordCallback,
                      nullptr)) {
    std::cerr << "Failed to open microphone device\n";
    return -1;
  }

  int32_t window_size = 512; // samples, please don't change

  int32_t offset = 0;
  std::vector<float> buffer;
  bool speech_started = false;

  auto started_time = std::chrono::steady_clock::now();

  // SherpaDisplay display;

  std::cout << "Started! Please speak\n";
  std::cout << "  - 停止语音输入 / 停止输入 -> Pause\n"
               "  - 开启语音输入 / 启动语音输入 -> Resume\n";

  while (!g_stop.load()) {
    {
      std::unique_lock<std::mutex> lock(mutex);
      while (samples_queue.empty() && !g_stop.load()) {
        condition_variable.wait(lock);
      }

      if (g_stop.load()) {
        break;
      }

      if (samples_queue.size() == 0) {
        continue;
      }

      const auto &s = samples_queue.front();
      if (!resampler.Get()) {
        buffer.insert(buffer.end(), s.begin(), s.end());
      } else {
        auto resampled = resampler.Resample(s.data(), s.size(), false);
        buffer.insert(buffer.end(), resampled.begin(), resampled.end());
      }

      samples_queue.pop();
    }

    for (; offset + window_size < buffer.size(); offset += window_size) {
      vad.AcceptWaveform(buffer.data() + offset, window_size);
      if (!speech_started && vad.IsDetected()) {
        speech_started = true;
        started_time = std::chrono::steady_clock::now();
      }
    }
    if (!speech_started) {
      if (buffer.size() > 10 * window_size) {
        offset -= buffer.size() - 10 * window_size;
        buffer = {buffer.end() - 10 * window_size, buffer.end()};
      }
    }

    auto current_time = std::chrono::steady_clock::now();
    const float elapsed_seconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(current_time -
                                                              started_time)
            .count() /
        1000.;

    if (speech_started && elapsed_seconds > 0.4) {
      OfflineStream stream = recognizer.CreateStream();
      stream.AcceptWaveform(sample_rate, buffer.data(), buffer.size());

      recognizer.Decode(&stream);

      OfflineRecognizerResult result = recognizer.GetResult(&stream);
      got_tmp_input(result.text);
      if (!g_paused.load()) {
        // ClearScreen(); // Avoid clearing console in GUI app
        std::cout << "recognizer.GetResult tmp=" << result.text << std::endl;
      }

      // Handle voice pause/resume commands; do not type for this branch anyway
      (void)HandleVoiceCommand(result.text);
      // display.UpdateText(result.text);
      // display.Display();

      started_time = std::chrono::steady_clock::now();
    }

    while (!vad.IsEmpty()) {
      auto segment = vad.Front();

      vad.Pop();

      OfflineStream stream = recognizer.CreateStream();
      stream.AcceptWaveform(sample_rate, segment.samples.data(),
                            segment.samples.size());

      recognizer.Decode(&stream);

      OfflineRecognizerResult result = recognizer.GetResult(&stream);

      (void)HandleVoiceCommand(result.text);

      if (!g_paused.load()) {
        // Replace the UTF-8 Chinese full stop (E3 80 82) with a comma.
        // std::replace works on single bytes; '。' is a multibyte UTF-8 sequence,
        // so we must replace the substring instead of a single char.
        std::string text = result.text;
        const std::string full_stop = "\xE3\x80\x82"; // "。"
        size_t pos = 0;
        while ((pos = text.find(full_stop, pos)) != std::string::npos) {
          text.replace(pos, full_stop.size(), ",");
          pos += 1; // advance past the inserted comma
        }
        std::cout << "typestr=" << result.text << std::endl;

        got_input(text);

        typestr_simple(text.c_str());
      } else {
        std::cout << "[PAUSE]=" << result.text << std::endl;
      }

      // display.UpdateText(result.text);
      // display.FinalizeCurrentSentence();
      // display.Display();

      buffer.clear();
      offset = 0;
      speech_started = false;
    }
  }

  return 0;
}

// ---------- Public API ----------
void VibeInputStart(const VibeInputOptions &opts) {
  bool expected = false;
  if (!g_running.compare_exchange_strong(expected, true)) {
    std::cerr << "VibeInput already running\n";
    return;
  }
  g_stop.store(false);
  g_worker = std::thread([opts]() {
    (void)WorkerMain(opts);
    g_running.store(false);
  });
}

void VibeInputStop() {
  if (!g_running.load()) return;
  g_stop.store(true);
  condition_variable.notify_one();
  if (g_worker.joinable()) {
    g_worker.join();
  }
  g_running.store(false);
}

void VibeInputTogglePause(const char *reason) {
  TogglePause(reason);
  sync_display();
}