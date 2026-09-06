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
    bool processed = false;
    if (ready_ && frames <= kBlockSize) {
        for (std::size_t i = 0; i < frames; ++i)
            input_[i] = std::isfinite(left[i]) ? left[i] * kInputGain : 0.0f;
        processed = processor_.Process(input_.data(), output_.data(), frames);
    }
    for (std::size_t i = 0; i < frames; ++i) {
        const float sample = OutputSample(processed && !bypass ? output_[i] : left[i]);
        out_left[i] = sample;
        out_right[i] = sample;
    }
}
