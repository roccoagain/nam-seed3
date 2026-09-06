#pragma once
#include <memory>

namespace nam {
class DSP;
}
enum class AmpId { Fender = 1, Vox = 2, Marshall = 3 };
const char *AmpName(AmpId id);
std::unique_ptr<nam::DSP> CreateAmpModel(AmpId id);
