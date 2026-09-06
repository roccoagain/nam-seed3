# Bundled model

`test_lstm.nam` is an unchanged copy of `example_models/lstm.nam` from
[NeuralAmpModelerCore at 20a04fc](https://github.com/sdatkinson/NeuralAmpModelerCore/tree/20a04fcf466dc4233730412b120e5bbad72402c3).
The upstream repository distributes this file under its MIT license, copied
verbatim to `LICENSE` in this directory. Keep that notice with redistributions,
including the generated model data.

- SHA-256: `df9f78c49f49c2bb32411df47e3f53746075adb206b92d017e06379d1e56234a`
- NAM format: `0.5.4`
- Architecture: mono LSTM, one layer, three hidden units, 70 weights (280 bytes as float32).
- Sample rate: 48,000 Hz.
- Upstream metadata: name `Test LSTM`, modeled by `Steve`, clean
  Darkglass Electronics Microtubes 900 v2.

This small upstream example is for integration and performance experiments.
Its metadata does not provide a validation ESR; no capture-quality claim is made.
It is quiet relative to bypass. Firmware uses unity input gain and 0.8 output
gain, with no automatic loudness normalization or dBu calibration. The metadata's
input/output levels remain in the source model for future calibration work.

`make model` converts this source to `build/generated/embedded_model_data.h`.
The converter rounds weights to float32 and writes exact hexadecimal literals;
the generated header records the source checksum. The firmware constructs the
upstream LSTM directly from that data, without JSON parsing or file access.

The current converter accepts only NAM 0.5.4/0.6.0, mono, single-layer 48 kHz
LSTMs with 1–8 hidden units. It rejects other configurations, invalid weight
counts, and nonfinite/out-of-range weights. This support range is a format
restriction, not a guarantee of real-time performance for every accepted model.
Replacing the bundled model requires updating this provenance and license record
and validating the new model on the target.
