#pragma once

#include "amp_models.h"
#include "nam_processor.h"
#include <array>
#include <cstddef>

class NamAudio {
public:
  static constexpr std::size_t kBlockSize = 48;
  static constexpr double kSampleRate = 48000.0;
  static constexpr float kInputGain = 1.0f;
  static constexpr float kOutputGain = 0.8f;

  // Call with audio stopped. Failure leaves the path in bypass.
  bool Init(AmpId amp = AmpId::Fender);
  // Left input to both outputs. Bypass still advances model state to allow
  // meaningful comparisons. Buffers must contain at least frames samples.
  void Process(const float *left, float *out_left, float *out_right,
               std::size_t frames, bool bypass);

private:
  NamProcessor processor_;
  bool ready_ = false;
  std::array<float, kBlockSize> input_{};
  std::array<float, kBlockSize> output_{};
};
