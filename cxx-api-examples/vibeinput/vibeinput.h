#ifndef VIBEINPUT_H_
#define VIBEINPUT_H_

#include <string>

enum class DenoiseMethod {
  None,
  RNNoise,
  GTCRN
};

enum class AsrModelType {
  SenseVoice,   // default: SenseVoice model
  FireRedAsr,   // FireRedAsr model (requires encoder + decoder)
  Paraformer    // Paraformer model (single model file)
};

struct VibeInputOptions {
  std::string vad_model;  // default: "silero_vad.int8.onnx"
  std::string asr_model;  // default: "model.int8.onnx" for SenseVoice
  std::string tokens;     // default: "tokens.txt"
  
  // ASR model type selection
  AsrModelType asr_model_type = AsrModelType::SenseVoice;
  
  // FireRedAsr specific: encoder and decoder paths
  std::string fire_red_encoder;  // e.g., "encoder.int8.onnx"
  std::string fire_red_decoder;  // e.g., "decoder.int8.onnx"
  
  // Number of threads for ASR model inference
  int num_threads = 4;
  
  // Denoise options
  DenoiseMethod denoise_method = DenoiseMethod::None;
  // For GTCRN, model path (will be resolved like other models if relative)
  std::string denoise_model; // default empty means use "gtcrn_simple.onnx"
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
