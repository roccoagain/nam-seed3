# Embedded LSTM adaptation

`nam-lstm.patch` applies to NAM Core revision
`20a04fcf466dc4233730412b120e5bbad72402c3`. `model.mk` copies the two source
files into `build/nam/NAM/` and applies the patch there. `libs/` stays unmodified.

The patch returns a view of hidden state, accepts an Eigen reference when
passing it between cells, and evaluates matrix multiplication into preallocated
storage. This removes temporary Eigen vector allocations from sample processing.
It also allows the embedded build to omit the static JSON LSTM registration;
the firmware calls the constructor directly.

`src/nam_lstm_activations.cpp` supplies only the upstream `using_fast_tanh` flag,
set false. LSTM sigmoid/tanh implementations are inline. The desktop activation
registry and its static heap allocations are not linked. Other architectures
and registry-based loading need a different build configuration.

`make test` compares the patched embedded path with a separate executable using
the unmodified upstream JSON loader, LSTM, and activation implementation. The
embedded path runs with Eigen heap allocation disabled after initialization.
Source licensing remains the NAM Core MIT license in `libs/NeuralAmpModelerCore/LICENSE`.
