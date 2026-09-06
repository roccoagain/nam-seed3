#pragma once

#include "NAM/dsp.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Fixed 3-channel, 23-layer A2-Lite WaveNet. Only configurations validated by
// convert_a2.py are accepted. Weights must outlive this object (embedded
// flash).
class A2Lite final : public nam::DSP {
  public:
    static constexpr std::size_t kWeights = 1871;
    explicit A2Lite(const float *weights);
    void Reset(double sample_rate, int max_block_size) override;
    void process(float **input, float **output, int frames) override;

  private:
    struct Layer {
        const float *weights;
        std::vector<float> history;
        uint32_t position = 0;
        uint32_t mask;
        int kernel;
        int dilation;
    };
    int PrewarmSamples() override { return 6347; }

    static void ProcessLayer(Layer &layer, float input, float *features, float *skip_sum);
    float ProcessHead(const float *skip_sum);
    float Sample(float input);

    const float *weights_;
    const float *head_;
    std::array<Layer, 23> layers_;
    std::array<float, 16 * 3> head_history_{};
    uint32_t head_position_ = 0;
};
