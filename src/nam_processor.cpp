#include "nam_processor.h"

#include "NAM/dsp.h"
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

static_assert(std::is_same<NAM_SAMPLE, float>::value,
              "NAM must use the same float samples as Daisy");

NamProcessor::NamProcessor() = default;
NamProcessor::~NamProcessor() = default;

bool NamProcessor::Prepare(std::unique_ptr<nam::DSP> model, double sample_rate,
                           std::size_t max_block_size) {
  if (!model || !std::isfinite(sample_rate) || sample_rate <= 0.0 ||
      max_block_size == 0 ||
      max_block_size >
          static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
      model->NumInputChannels() != 1 || model->NumOutputChannels() != 1 ||
      model->GetExpectedSampleRate() != sample_rate) {
    return false;
  }
  try {
    model->ResetAndPrewarm(sample_rate, static_cast<int>(max_block_size));
  } catch (...) {
    return false;
  }
  model_ = std::move(model);
  max_block_size_ = max_block_size;
  return true;
}

bool NamProcessor::Process(float *input, float *output, std::size_t frames) {
  if (!model_ || !input || !output || input == output || frames == 0 ||
      frames > max_block_size_) {
    return false;
  }
  model_->process(&input, &output, static_cast<int>(frames));
  return true;
}
