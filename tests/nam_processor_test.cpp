#include "NAM/dsp.h"
#include "NAM/lstm.h"
#include "nam_processor.h"
#include "embedded_model.h"
#include "audio_stimulus.h"
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

// A small synthetic recurrent model, not an amp capture. Nonzero weights
// exercise state retention and the upstream float sample ABI across blocks.
static std::unique_ptr<nam::DSP> MakeModel() {
  std::vector<float> weights(16, 0.1f);
  return std::make_unique<nam::lstm::LSTM>(1, 1, 1, 1, 1, weights, 48000.0);
}

class FailingModel : public nam::DSP {
public:
  FailingModel() : DSP(1, 1, 48000.0) {}
  void Reset(double, int) override { throw std::runtime_error("test failure"); }
};

int main() {
  NamProcessor processor;
  std::array<float, 48> input{}, output{}, expected{};
  output.fill(123.0f);
  assert(!processor.Process(input.data(), output.data(), 48));
  assert(output[0] == 123.0f);
  assert(!processor.Prepare(nullptr, 48000.0, 48));
  assert(!processor.Prepare(MakeModel(), 44100.0, 48));
  assert(!processor.Prepare(MakeModel(), 48000.0, 0));
  assert(processor.Prepare(CreateEmbeddedModel(), 48000.0, 48));
  assert(!processor.Prepare(MakeModel(),
                            std::numeric_limits<double>::quiet_NaN(), 48));
  assert(!processor.Prepare(std::make_unique<nam::DSP>(2, 1, 48000.0), 48000.0,
                            48));
  assert(
      !processor.Prepare(std::make_unique<nam::DSP>(1, 1, -1.0), 48000.0, 48));
  assert(processor.Prepare(MakeModel(), 48000.0, 48));
  auto reference = MakeModel();
  reference->ResetAndPrewarm(48000.0, 48);
  assert(!processor.Prepare(std::make_unique<FailingModel>(), 48000.0, 48));
  assert(!processor.Process(input.data(), output.data(), 49));
  assert(!processor.Process(nullptr, output.data(), 48));
  assert(!processor.Process(input.data(), input.data(), 48));
  assert(output[0] == 123.0f);
  for (int block = 0; block < 10; ++block) {
    const int frames = block % 2 ? 17 : 48;
    for (int i = 0; i < frames; ++i)
      input[i] = std::sin(static_cast<float>(block * 48 + i) * 0.1f);
    float *in = input.data();
    float *out = expected.data();
    reference->process(&in, &out, frames);
    assert(processor.Process(input.data(), output.data(), frames));
    for (int i = 0; i < frames; ++i)
      assert(std::isfinite(output[i]) &&
             std::fabs(output[i] - expected[i]) < 1e-6f);
  }
  // Keep the bundled small LSTM covered against the unmodified JSON loader.
  assert(processor.Prepare(CreateEmbeddedModel(), 48000.0, 48));
  std::ifstream file("build/tests/reference.f32", std::ios::binary);
  for (int offset = 0; offset < kTestSamples; offset += 48) {
    file.read(reinterpret_cast<char *>(expected.data()), sizeof(expected));
    assert(file.gcount() == sizeof(expected));
    for (int i = 0; i < 48; ++i) input[i] = TestInput(offset + i);
    assert(processor.Process(input.data(), output.data(), 48));
    for (int i = 0; i < 48; ++i)
      assert(std::fabs(output[i] - expected[i]) < 1e-6f);
  }
}
