// A2-Lite equations and weight ordering follow NeuralAmpModelerCore's WaveNet.
// See patches/NAM-LICENSE for the upstream MIT notice.
#include "a2_lite.h"
#include <algorithm>

namespace {
constexpr int kChannels = 3;
constexpr int kWeightsPerTap = kChannels * kChannels;
constexpr int kLayerTailWeights = 18;
constexpr int kHeadTaps = 16;
constexpr int kHeadMask = kHeadTaps - 1;
constexpr int kHeadBiasOffset = kHeadTaps * kChannels;
constexpr int kHeadScaleOffset = kHeadBiasOffset + 1;

constexpr int kKernels[] = {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 15, 15, 6, 6, 6, 6, 6, 6, 6};
constexpr int kDilations[] = {1, 3, 7, 17, 41, 101, 239, 1, 3, 7, 17, 41, 101, 239, 1, 13, 1, 3, 7, 17, 41, 101, 239};
} // namespace

A2Lite::A2Lite(const float *weights) : DSP(1, 1, 48000.0), weights_(weights) {
    const float *next = weights + kChannels;
    for (std::size_t i = 0; i < layers_.size(); ++i) {
        auto &layer = layers_[i];
        layer.kernel = kKernels[i];
        layer.dilation = kDilations[i];
        uint32_t history_size = 1;
        // A power-of-two ring buffer lets the mask wrap each delayed tap.
        while (history_size <= static_cast<uint32_t>((layer.kernel - 1) * layer.dilation))
            history_size *= 2;
        layer.mask = history_size - 1;
        layer.history.resize(history_size * kChannels, 0.0f);
        layer.weights = next;
        next += layer.kernel * kWeightsPerTap + kLayerTailWeights;
    }
    head_ = next;
}

void A2Lite::Reset(double sample_rate, int max_block_size) {
    for (auto &layer : layers_) {
        std::fill(layer.history.begin(), layer.history.end(), 0.0f);
        layer.position = 0;
    }
    head_history_.fill(0.0f);
    head_position_ = 0;
    DSP::Reset(sample_rate, max_block_size);
}

void A2Lite::ProcessLayer(Layer &layer, float input, float *features, float *skip_sum) {
    float *current = &layer.history[layer.position * kChannels];
    std::copy(features, features + kChannels, current);

    // After the convolution: bias[3], input conditioning[3],
    // residual projection[9], and residual bias[3].
    const float *bias = layer.weights + layer.kernel * kWeightsPerTap;
    const float *conditioning = bias + kChannels;
    const float *residual_weights = conditioning + kChannels;
    const float *residual_bias = residual_weights + kWeightsPerTap;
    float activation[kChannels] = {bias[0], bias[1], bias[2]};
    const float *tap_weights = layer.weights;
    for (int tap = 0; tap < layer.kernel; ++tap, tap_weights += kWeightsPerTap) {
        const uint32_t delay = static_cast<uint32_t>((layer.kernel - 1 - tap) * layer.dilation);
        const uint32_t position = (layer.position - delay) & layer.mask;
        const float *history = &layer.history[position * kChannels];
        // Preserve the upstream accumulation order for each output channel.
        activation[0] += tap_weights[0] * history[0];
        activation[0] += tap_weights[3] * history[1];
        activation[0] += tap_weights[6] * history[2];
        activation[1] += tap_weights[1] * history[0];
        activation[1] += tap_weights[4] * history[1];
        activation[1] += tap_weights[7] * history[2];
        activation[2] += tap_weights[2] * history[0];
        activation[2] += tap_weights[5] * history[1];
        activation[2] += tap_weights[8] * history[2];
    }

    for (int channel = 0; channel < kChannels; ++channel) {
        activation[channel] += conditioning[channel] * input;
        activation[channel] = activation[channel] >= 0.0f ? activation[channel] : 0.01f * activation[channel];
        skip_sum[channel] += activation[channel];
    }
    for (int channel = 0; channel < kChannels; ++channel) {
        const float *projection = residual_weights + channel * kChannels;
        features[channel] += residual_bias[channel] + projection[0] * activation[0] + projection[1] * activation[1] + projection[2] * activation[2];
    }
    layer.position = (layer.position + 1) & layer.mask;
}

float A2Lite::ProcessHead(const float *skip_sum) {
    std::copy(skip_sum, skip_sum + kChannels, &head_history_[head_position_ * kChannels]);
    float output = head_[kHeadBiasOffset];
    for (uint32_t tap = 0; tap < kHeadTaps; ++tap) {
        const uint32_t position = (head_position_ - kHeadMask + tap) & kHeadMask;
        const float *history = &head_history_[position * kChannels];
        for (int channel = 0; channel < kChannels; ++channel)
            output += head_[tap * kChannels + channel] * history[channel];
    }
    head_position_ = (head_position_ + 1) & kHeadMask;
    return output * head_[kHeadScaleOffset];
}

float A2Lite::Sample(float input) {
    float features[kChannels] = {weights_[0] * input, weights_[1] * input, weights_[2] * input};
    float skip_sum[kChannels] = {};
    for (auto &layer : layers_)
        ProcessLayer(layer, input, features, skip_sum);
    return ProcessHead(skip_sum);
}

void A2Lite::process(float **input, float **output, int frames) {
    for (int i = 0; i < frames; ++i)
        output[0][i] = Sample(input[0][i]);
}
