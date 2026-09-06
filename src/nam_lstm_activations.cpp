#include "NAM/activations.h"

// The supported LSTM uses inline sigmoid/tanh, requiring only this flag.
// Do not link the desktop activation registry and its static heap allocations.
// Other activation APIs intentionally remain unavailable in this firmware.
bool nam::activations::Activation::using_fast_tanh = false;
