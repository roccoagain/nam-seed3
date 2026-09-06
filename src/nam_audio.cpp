#include "nam_audio.h"
#include "NAM/dsp.h"
#include <algorithm>
#include <cmath>

namespace {
float OutputSample(float sample) {
    if (!std::isfinite(sample))
        return 0.0f;
    return std::max(-1.0f, std::min(1.0f, sample * NamAudio::kOutputGain));
}
} // namespace

bool NamAudio::Init(AmpId amp) {
    ready_ = false;
    try {
        ready_ = processor_.Prepare(CreateAmpModel(amp), kSampleRate, kBlockSize);
    } catch (...) {
        // Allocation/construction failure must not prevent audio startup.
    }
    return ready_;
}

void NamAudio::Process(const float *left, float *out_left, float *out_right, std::size_t frames, bool bypass) {
    const bool processed = ProcessModel(left, frames);
    const float *source = processed && !bypass ? output_.data() : left;
    for (std::size_t i = 0; i < frames; ++i) {
        const float sample = OutputSample(source[i]);
        out_left[i] = sample;
        out_right[i] = sample;
    }
}

bool NamAudio::ProcessModel(const float *input, std::size_t frames) {
    if (!ready_ || frames > kBlockSize)
        return false;

    for (std::size_t i = 0; i < frames; ++i)
        input_[i] = std::isfinite(input[i]) ? input[i] * kInputGain : 0.0f;
    return processor_.Process(input_.data(), output_.data(), frames);
}
