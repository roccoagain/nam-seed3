// A2-Lite equations and weight ordering follow NeuralAmpModelerCore's WaveNet.
// See patches/NAM-LICENSE for the upstream MIT notice.
#include "a2_lite.h"
#include <algorithm>

namespace {
constexpr int kKernels[] = {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 15, 15, 6, 6, 6, 6, 6, 6, 6};
constexpr int kDilations[] = {1, 3, 7, 17, 41, 101, 239, 1, 3, 7, 17, 41, 101, 239, 1, 13, 1, 3, 7, 17, 41, 101, 239};
} // namespace

A2Lite::A2Lite(const float *weights) : DSP(1, 1, 48000.0), weights_(weights) {
    const float *next = weights + 3;
    for (std::size_t i = 0; i < layers_.size(); ++i) {
        auto &layer = layers_[i];
        layer.kernel = kKernels[i];
        layer.dilation = kDilations[i];
        uint32_t size = 1;
        while (size <= static_cast<uint32_t>((layer.kernel - 1) * layer.dilation))
            size *= 2;
        layer.mask = size - 1;
        layer.history.resize(size * 3, 0.0f);
        layer.weights = next;
        next += layer.kernel * 9 + 18;
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

float A2Lite::Sample(float input) {
    float x[3] = {weights_[0] * input, weights_[1] * input, weights_[2] * input};
    float sum[3] = {};
    for (auto &layer : layers_) {
        float *current = &layer.history[layer.position * 3];
        std::copy(x, x + 3, current);
        const float *tail = layer.weights + layer.kernel * 9;
        float z[3] = {tail[0], tail[1], tail[2]};
        const float *w = layer.weights;
        for (int tap = 0; tap < layer.kernel; ++tap, w += 9) {
            const uint32_t pos = (layer.position - static_cast<uint32_t>((layer.kernel - 1 - tap) * layer.dilation)) & layer.mask;
            const float *h = &layer.history[pos * 3];
            z[0] += w[0] * h[0];
            z[0] += w[3] * h[1];
            z[0] += w[6] * h[2];
            z[1] += w[1] * h[0];
            z[1] += w[4] * h[1];
            z[1] += w[7] * h[2];
            z[2] += w[2] * h[0];
            z[2] += w[5] * h[1];
            z[2] += w[8] * h[2];
        }
        for (int c = 0; c < 3; ++c) {
            z[c] += tail[3 + c] * input;
            z[c] = z[c] >= 0.0f ? z[c] : 0.01f * z[c];
            sum[c] += z[c];
        }
        for (int c = 0; c < 3; ++c)
            x[c] += tail[15 + c] + tail[6 + c * 3] * z[0] + tail[7 + c * 3] * z[1] + tail[8 + c * 3] * z[2];
        layer.position = (layer.position + 1) & layer.mask;
    }
    std::copy(sum, sum + 3, &head_history_[head_position_ * 3]);
    float output = head_[48];
    for (uint32_t tap = 0; tap < 16; ++tap) {
        const float *h = &head_history_[((head_position_ - 15 + tap) & 15) * 3];
        for (int c = 0; c < 3; ++c)
            output += head_[tap * 3 + c] * h[c];
    }
    head_position_ = (head_position_ + 1) & 15;
    return output * head_[49];
}

void A2Lite::process(float **input, float **output, int frames) {
    for (int i = 0; i < frames; ++i)
        output[0][i] = Sample(input[0][i]);
}
