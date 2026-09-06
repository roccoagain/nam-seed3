#pragma once

#include <cstddef>
#include <memory>

namespace nam {
class DSP;
}

// Owns one mono model. Prepare/destruction require audio to be stopped;
// Process is the only method intended for the audio callback.
class NamProcessor {
  public:
    NamProcessor();
    ~NamProcessor();

    NamProcessor(const NamProcessor &) = delete;
    NamProcessor &operator=(const NamProcessor &) = delete;

    // Takes ownership, validates format, then resets and prewarms the model.
    // Returns false on failure, preserving any previously prepared model.
    // Unknown model sample rates are rejected; no resampling is performed.
    bool Prepare(std::unique_ptr<nam::DSP> model, double sample_rate, std::size_t max_block_size);

    // Input and output must be distinct buffers of at least frames samples.
    // Returns false without writing output if unprepared or arguments invalid.
    // No wrapper allocations; model runtime behavior must be validated
    // separately.
    bool Process(float *input, float *output, std::size_t frames);

  private:
    std::unique_ptr<nam::DSP> model_;
    std::size_t max_block_size_ = 0;
};
