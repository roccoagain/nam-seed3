#include "NAM/get_dsp.h"
#include "audio_stimulus.h"
#include <array>
#include <fstream>

int main() {
  // Deliberately use the unmodified upstream JSON loader and LSTM source.
  auto model = nam::get_dsp(std::filesystem::path("tests/fixtures/test_lstm.nam"));
  model->ResetAndPrewarm(48000.0, 48);
  std::ofstream file("build/tests/reference.f32", std::ios::binary);
  std::array<float, 48> input{}, output{};
  for (int offset = 0; offset < kTestSamples; offset += 48) {
    for (int i = 0; i < 48; ++i)
      input[i] = TestInput(offset + i);
    float *in = input.data();
    float *out = output.data();
    model->process(&in, &out, 48);
    file.write(reinterpret_cast<const char *>(output.data()), sizeof(output));
  }
  return file.good() ? 0 : 1;
}
