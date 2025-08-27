#ifndef VIBEINPUT_H_
#define VIBEINPUT_H_

#include <string>

struct VibeInputOptions {
  std::string vad_model;  // default: "silero_vad.int8.onnx"
  std::string asr_model;  // default: "model.int8.onnx"
  std::string tokens;     // default: "tokens.txt"
};

// Start the background speech worker. No-op if already running.
void VibeInputStart(const VibeInputOptions &opts = VibeInputOptions{});

// Stop the background speech worker and join the thread.
void VibeInputStop();

// Toggle pause/resume typing by voice; optional reason for logging.
void VibeInputTogglePause(const char *reason = nullptr);

// is vibe input paused
bool VibeInputIsPaused();

extern void got_tmp_input(const std::string& txt);
extern void got_input(const std::string& txt);



#endif  // VIBEINPUT_H_
