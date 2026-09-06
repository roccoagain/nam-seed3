# Bundled test model

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

This small upstream example is used only by host regression tests. It is not
part of the three-amp firmware.

`scripts/convert_lstm.py` converts it to
`build/generated/embedded_model_data.h`. The converter supports NAM 0.5.4/0.6.0,
mono, single-layer 48 kHz LSTMs with 1–8 hidden units. It rounds weights to
float32, emits exact hexadecimal literals, and records the source checksum.
The test helper in `tests/support/` constructs the LSTM from this data.

Keep this provenance and the accompanying MIT license with the fixture.
