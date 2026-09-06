#include "embedded_model.h"
#include "NAM/lstm.h"
#include "embedded_model_data.h"
#include <iterator>
#include <vector>

std::unique_ptr<nam::DSP> CreateEmbeddedModel() {
  std::vector<float> weights(std::begin(embedded_model::kWeights),
                             std::end(embedded_model::kWeights));
  return std::make_unique<nam::lstm::LSTM>(1, 1, 1, 1,
                                           embedded_model::kHiddenSize, weights,
                                           embedded_model::kSampleRate);
}
