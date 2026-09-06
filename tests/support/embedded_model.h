#pragma once

#include <memory>
namespace nam {
class DSP;
}

// Startup only: constructs from generated model data, without JSON or file I/O.
// May throw on allocation failure; the caller must handle initialization
// failure.
std::unique_ptr<nam::DSP> CreateEmbeddedModel();
