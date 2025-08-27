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

// Toggle hotkey pause/resume (full stop of VAD/ASR); optional reason for logging.
void VibeInputToggleHotkeyPause(const char *reason = nullptr);

// Toggle voice pause/resume (continue VAD/ASR, only block typing); optional reason for logging.
void VibeInputToggleVoicePause(const char *reason = nullptr);

// Combined paused state (either hotkey or voice paused)
bool VibeInputIsPaused();

// Query specific pause states
bool VibeInputIsHotkeyPaused();
bool VibeInputIsVoicePaused();

extern void got_tmp_input(const std::string& txt);
extern void got_input(const std::string& txt);
extern void sync_display();



#endif  // VIBEINPUT_H_
