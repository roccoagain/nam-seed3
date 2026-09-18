// A2-Lite equations and weight ordering follow NeuralAmpModelerCore's WaveNet.
// See NAM-LICENSE for the upstream MIT notice.
#include "models/a2_lite.h"
#include <algorithm>

using namespace a2_lite;

namespace {
constexpr uint32_t kHeadMask = kHeadTaps - 1;
static_assert((kHeadTaps & kHeadMask) == 0, "head history must be a power of two");
} // namespace

A2Lite::A2Lite(const float *weights) : DSP(1, 1, 48000.0), input_projection_(weights) {
    const float *next = weights + kChannels;
    for (std::size_t i = 0; i < layers_.size(); ++i) {
        auto &layer = layers_[i];
        layer.kernel_size = kKernelSizes[i];
        layer.dilation = kDilations[i];
        layer.taps = next;
        layer.bias = layer.taps + layer.kernel_size * kWeightsPerTap;
        layer.conditioning = layer.bias + kChannels;
        layer.residual = layer.conditioning + kChannels;
        layer.residual_bias = layer.residual + kChannels * kChannels;
        next += LayerWeightCount(layer.kernel_size);

        // A power-of-two ring buffer lets the mask wrap each delayed tap.
        uint32_t history_size = 1;
        while (history_size <= static_cast<uint32_t>((layer.kernel_size - 1) * layer.dilation))
            history_size *= 2;
        layer.history_wrap_mask = history_size - 1;
        layer.history.resize(history_size * kChannels, 0.0f);
    }
    head_taps_ = next;
    head_bias_ = next[kHeadTaps * kChannels];
    head_scale_ = next[kHeadTaps * kChannels + 1];
}

void A2Lite::Reset(double sample_rate_hz, int max_block_size) {
    for (auto &layer : layers_) {
        std::fill(layer.history.begin(), layer.history.end(), 0.0f);
        layer.history_write_index = 0;
    }
    head_history_.fill(0.0f);
    head_history_write_index_ = 0;
    DSP::Reset(sample_rate_hz, max_block_size);
}

void A2Lite::ProcessLayer(Layer &layer, float conditioning_sample, float *residual_features, float *skip_accumulator) {
    std::copy(residual_features, residual_features + kChannels, &layer.history[layer.history_write_index * kChannels]);

    // Dilated convolution over the delay line. Each output channel sums its
    // inputs in the same order as upstream so results match bit for bit.
    float activation[kChannels] = {layer.bias[0], layer.bias[1], layer.bias[2]};
    const float *tap_weights = layer.taps;
    for (int tap = 0; tap < layer.kernel_size; ++tap, tap_weights += kWeightsPerTap) {
        const uint32_t delay = static_cast<uint32_t>((layer.kernel_size - 1 - tap) * layer.dilation);
        const float *history = &layer.history[((layer.history_write_index - delay) & layer.history_wrap_mask) * kChannels];
        for (int out = 0; out < kChannels; ++out)
            for (int in = 0; in < kChannels; ++in)
                activation[out] += tap_weights[in * kChannels + out] * history[in];
    }

    // Input conditioning, leaky ReLU, skip connection.
    for (int channel = 0; channel < kChannels; ++channel) {
        activation[channel] += layer.conditioning[channel] * conditioning_sample;
        activation[channel] = activation[channel] >= 0.0f ? activation[channel] : 0.01f * activation[channel];
        skip_accumulator[channel] += activation[channel];
    }
    // 1x1 residual projection back onto the layer input.
    for (int out = 0; out < kChannels; ++out) {
        const float *projection = layer.residual + out * kChannels;
        residual_features[out] += layer.residual_bias[out] + projection[0] * activation[0] + projection[1] * activation[1] + projection[2] * activation[2];
    }
    layer.history_write_index = (layer.history_write_index + 1) & layer.history_wrap_mask;
}

float A2Lite::ProcessHead(const float *skip_accumulator) {
    std::copy(skip_accumulator, skip_accumulator + kChannels, &head_history_[head_history_write_index_ * kChannels]);
    float output = head_bias_;
    for (uint32_t tap = 0; tap < kHeadTaps; ++tap) {
        const uint32_t delay = kHeadMask - tap;
        const float *history = &head_history_[((head_history_write_index_ - delay) & kHeadMask) * kChannels];
        for (int channel = 0; channel < kChannels; ++channel)
            output += head_taps_[tap * kChannels + channel] * history[channel];
    }
    head_history_write_index_ = (head_history_write_index_ + 1) & kHeadMask;
    return output * head_scale_;
}

float A2Lite::ProcessSample(float input_sample) {
    float residual_features[kChannels] = {input_projection_[0] * input_sample, input_projection_[1] * input_sample, input_projection_[2] * input_sample};
    float skip_accumulator[kChannels] = {};
    for (auto &layer : layers_)
        ProcessLayer(layer, input_sample, residual_features, skip_accumulator);
    return ProcessHead(skip_accumulator);
}

void A2Lite::process(float **input, float **output, int frame_count) {
    for (int i = 0; i < frame_count; ++i)
        output[0][i] = ProcessSample(input[0][i]);
}
