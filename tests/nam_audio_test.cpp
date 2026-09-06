#include "NAM/dsp.h"
#include "audio_stimulus.h"
#include "nam_audio.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

int main() {
  std::ifstream file("build/tests/reference.f32", std::ios::binary);
  std::vector<float> reference(kTestSamples);
  file.read(reinterpret_cast<char *>(reference.data()),
            reference.size() * sizeof(float));
  assert(file.gcount() ==
         static_cast<std::streamsize>(reference.size() * sizeof(float)));
  NamAudio audio;
  std::array<float, 49> input{}, left{}, right{};
  input.fill(0.5f);
  // Uninitialized and oversized blocks must bypass, and fully write outputs.
  audio.Process(input.data(), left.data(), right.data(), 49, false);
  assert(left[48] == 0.4f && right[48] == 0.4f);
  assert(audio.Init());
  audio.Process(input.data(), left.data(), right.data(), 49, false);
  assert(left[48] == 0.4f && right[48] == 0.4f);

  float max_error = 0.0f;
  // Disable Eigen heap allocation after initialization, including bypass.
  Eigen::internal::set_is_malloc_allowed(false);
  int offset = 0;
  while (offset < kTestSamples) {
    const int frames =
        std::min(offset % 3 == 0 ? 48 : 17, kTestSamples - offset);
    for (int i = 0; i < frames; ++i)
      input[i] = TestInput(offset + i);
    const bool bypass = offset >= 12000 && offset < 16000;
    left.fill(123.0f);
    right.fill(123.0f);
    audio.Process(input.data(), left.data(), right.data(), frames, bypass);
    for (int i = 0; i < frames; ++i) {
      const float expected = std::clamp(
          (bypass ? input[i] : reference[offset + i]) * NamAudio::kOutputGain,
          -1.0f, 1.0f);
      assert(std::isfinite(left[i]) && left[i] == right[i]);
      max_error = std::max(max_error, std::fabs(left[i] - expected));
      assert(std::fabs(left[i] - expected) < 1e-5f);
    }
    assert(left[frames] == 123.0f && right[frames] == 123.0f);
    offset += frames;
  }
  input[0] = std::numeric_limits<float>::quiet_NaN();
  input[1] = std::numeric_limits<float>::infinity();
  input[2] = 100.0f;
  audio.Process(input.data(), left.data(), right.data(), 3, true);
  assert(left[0] == 0.0f && left[1] == 0.0f && left[2] == 1.0f);
  audio.Process(input.data(), left.data(), right.data(), 3, false);
  assert(std::isfinite(left[0]) && std::isfinite(left[1]));
  Eigen::internal::set_is_malloc_allowed(true);
  std::cout << "NAM audio: upstream max error=" << max_error
            << "; no Eigen processing allocations\n";
}
