#pragma once

#include "NAM/dsp.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Shape of the A2-Lite WaveNet. convert_a2.py validates that every embedded
// model matches it, so the engine never parses configuration at runtime.
namespace a2_lite {
constexpr int kChannels = 3;
constexpr int kLayers = 23;
constexpr int kHeadTaps = 16;
constexpr std::array<int, kLayers> kKernelSizes = {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 15, 15, 6, 6, 6, 6, 6, 6, 6};
constexpr std::array<int, kLayers> kDilations = {1, 3, 7, 17, 41, 101, 239, 1, 3, 7, 17, 41, 101, 239, 1, 13, 1, 3, 7, 17, 41, 101, 239};

// Weight stream produced by convert_a2.py:
//   input projection[kChannels]
//   per layer: taps[kernel_size][in][out], bias[kChannels], conditioning[kChannels],
//              residual[out][in], residual bias[kChannels]
//   head taps[kHeadTaps][kChannels], head bias, output scale
constexpr int kWeightsPerTap = kChannels * kChannels;
constexpr int kLayerTailWeights = 3 * kChannels + kChannels * kChannels;

constexpr int LayerWeightCount(int kernel_size) { return kernel_size * kWeightsPerTap + kLayerTailWeights; }

constexpr std::size_t WeightCount() {
    std::size_t count = kChannels;
    for (int kernel_size : kKernelSizes)
        count += LayerWeightCount(kernel_size);
    return count + kHeadTaps * kChannels + 2;
}

// Samples needed to fill every delay line; matches upstream prewarming.
constexpr int ReceptiveField() {
    int samples = kHeadTaps;
    for (int i = 0; i < kLayers; ++i)
        samples += (kKernelSizes[i] - 1) * kDilations[i];
    return samples;
}
} // namespace a2_lite

// Fixed-shape A2-Lite inference. Weights must outlive this object (embedded
// flash).
class A2Lite final : public nam::DSP {
  public:
    static constexpr std::size_t kWeights = a2_lite::WeightCount();
    explicit A2Lite(const float *weights);
    void Reset(double sample_rate_hz, int max_block_size) override;
    void process(float **input, float **output, int frame_count) override;

  private:
    struct Layer {
        // Views into the weight stream; see the layout above.
        const float *taps = nullptr;
        const float *bias = nullptr;
        const float *conditioning = nullptr;
        const float *residual = nullptr;
        const float *residual_bias = nullptr;
        int kernel_size = 0;
        int dilation = 0;
        // Power-of-two ring buffer of [history_size][kChannels] samples.
        std::vector<float> history;
        uint32_t history_write_index = 0;
        uint32_t history_wrap_mask = 0;
    };

    int GetPrewarmSamples() override { return a2_lite::ReceptiveField(); }
    static void ProcessLayer(Layer &layer, float conditioning_sample, float *residual_features, float *skip_accumulator);
    float ProcessHead(const float *skip_accumulator);
    float ProcessSample(float input_sample);

    const float *input_projection_;
    std::array<Layer, a2_lite::kLayers> layers_;
    const float *head_taps_;
    float head_bias_;
    float head_scale_;
    std::array<float, a2_lite::kHeadTaps * a2_lite::kChannels> head_history_{};
    uint32_t head_history_write_index_ = 0;
};
