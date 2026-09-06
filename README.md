# Seed3 experiments

A C++ workspace for experimenting with audio processing on the Daisy Seed3,
using libDaisy. Firmware behavior will change as experiments develop; see
`src/main.cpp` for the current implementation.

## Setup (macOS)

Install [Homebrew](https://brew.sh) and Apple's Command Line Tools
(`xcode-select --install`), then run from the project root:

```sh
make install
make
```

`make install` installs the Homebrew `gcc-arm-embedded` cask and `dfu-util`
formula if missing, and downloads pinned libDaisy and NeuralAmpModelerCore
revisions and their submodules into `libs/`. The compiler installer may request your administrator
password. Existing packages are reused without upgrading them.

The build uses the ARM compiler on your `PATH`; its version is not pinned.
Run `make clean` before rebuilding after a compiler upgrade.

For formatting and clangd editor support, also install:

```sh
brew install clang-format compiledb
```

## Make targets

| Command | Purpose |
| --- | --- |
| `make install` | Install build/upload tools and fetch pinned dependencies. |
| `make` or `make build` | Build libDaisy, NAM Core, and firmware incrementally. |
| `make model` | Generate embedded model data and prepare the LSTM adaptation. |
| `make test` | Run host NAM processor tests with address/undefined sanitizers. |
| `make upload` | Build and upload firmware over USB DFU. |
| `make program-dfu` | Alias for `make upload`. |
| `make monitor` | Open USB serial in `screen` at 115200. |
| `make format` | Format C/C++ files under `src/` using `.clang-format`. |
| `make compiledb` | Generate `compile_commands.json` for clangd without compiling. |
| `make clean` | Remove firmware and libDaisy build outputs; retain dependencies. |
| `make help` | Show available commands. |

Use `make JOBS=8` to change libDaisy build concurrency (default: 4).

## Upload and monitor

USB serial logging is always enabled. Startup does not wait for a computer
connection; open `make monitor` to view model status and audio callback counts.

Connect the Seed3 using a USB-C **data** cable. Hold **BOOT**, press and release
**RESET**, then release **BOOT** to enter DFU mode. With only the intended DFU
device connected, run:

```sh
make upload
```

Uploading replaces the firmware on the board. Press **RESET** if needed after
uploading to start the application.

To view USB serial output from firmware that enables it, run in an interactive
terminal while the board is running the application, outside DFU mode:

```sh
make monitor
```

The monitor opens the single `/dev/cu.usbmodem*` port. If multiple ports exist,
it lists them so you can select one explicitly:

```sh
make monitor PORT=/dev/cu.usbmodemYOUR_PORT
```

Replace the example with the actual port name. Exit with **Ctrl-A**, release,
then **K**, then **Y** to confirm. Monitoring does not build or upload firmware.

## VS Code / clangd

Open the project root in VS Code with the clangd extension installed, then run:

```sh
make compiledb
```

Run **clangd: Restart language server** from the command palette after initial
setup. Regenerate the database when adding or removing source files, changing
build flags, or changing compiler versions; ordinary code edits do not require it.
Workspace settings let clangd query Homebrew's ARM compiler for standard headers.

## Layout

- `src/`: application source.
- `models/`: bundled source model, license, and provenance.
- `patches/`: reviewed embedded adaptations applied to generated dependency copies.
- `libs/`: downloaded dependencies, excluded from Git.
- `build/`: generated firmware and debugging files, excluded from Git.
- `scripts/`: dependency installation and serial monitor helpers.
- `tests/`: host tests for the NAM processor boundary.
- `Makefile`: developer commands.
- `firmware.mk`: application configuration using libDaisy's build rules.
- `model.mk`: shared model/source generation rules for firmware and host tests.

The generated compilation database and clangd cache are also excluded from Git.

## NAM audio

Firmware processes the left input through the bundled `Test LSTM` model and
duplicates the result to both outputs. The right input is unused. Processing
runs at 48 kHz with 48-sample blocks, unity input gain, and 0.8 output gain.
Output is clamped to [-1, 1]; nonfinite input/output samples are silenced.
The model is an upstream integration example, not a finished amp preset;
see [model provenance and license](models/README.md).

Model construction, allocation, and prewarming happen before audio starts.
If initialization fails, the firmware starts in clean left-input bypass and
reports `model=failed`. USB logging never waits for a computer connection.
To compare with clean audio, set `kBypass = true` in `src/main.cpp` and rebuild.
Bypass uses the same output gain and continues advancing a successfully loaded
model's state. Gain constants are in `src/nam_audio.h`.

`make build` automatically converts the checked-in `.nam` file into a generated
C++ header containing float32 weights. No SD card, download, or file parser is
needed at startup. To invoke the converter directly:

```sh
python3 scripts/convert_model.py models/test_lstm.nam build/generated/embedded_model_data.h
```

The converter deliberately supports only small, mono, single-layer 48 kHz LSTMs.
See `models/README.md` for exact limits; arbitrary `.nam` models are not supported.

## NAM build and validation

The build compiles [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore)
at `20a04fcf466dc4233730412b120e5bbad72402c3`, the revision used by the
[TONE3000 Daisy reference](https://github.com/tone-3000/nam-pedal).
Eigen and AudioDSPTools are pinned by that revision's submodules; nlohmann JSON
is included in the Core checkout. Dependency licenses remain in `libs/`.
Run `make install` again when updating an existing workspace.

`firmware.mk` builds the engine into `build/libnam.a` using C++17,
`NAM_SAMPLE_FLOAT`, and `NAM_USE_INLINE_GEMM`. Exceptions are enabled because
upstream configuration/initialization code throws. Only the LSTM inference
sources are compiled for this model. Fast-math and approximate activations are
not enabled. Application code, NAM inference, and libDaisy use GCC's `-Os`
optimization for code size. Run `make clean` before rebuilding after changing
optimization flags so cached dependency objects are rebuilt too.
The small [LSTM adaptation](patches/README.md) removes per-sample Eigen
temporaries and unused desktop registries; it is applied to copies under `build/`.

`src/nam_processor.h` provides `NamProcessor`, which owns an already constructed
`nam::DSP`. With audio stopped, call `Prepare(model, sample_rate, max_block_size)`
to validate mono I/O and an exact known sample rate, then reset and prewarm it.
Preparation may allocate; failure returns false and preserves the previous model.
During audio processing, `Process(input, output, frames)` accepts distinct mono
float buffers up to the prepared block size. Invalid calls return false without
writing output. The wrapper makes no processing allocations; each actual model
still needs allocation and timing validation. Preparation, destruction, and
processing must not run concurrently. Processing exceptions are not caught.

`make test` checks conversion/rejection behavior, invalid preparation, bypass,
stereo duplication, output bounds, and recurrent state across block sizes.
A separate unmodified upstream executable loads the source JSON and generates
reference output for silence, an impulse, a step, and multitone input. The embedded
path is compared against it with Eigen allocations forbidden after initialization.
Tests use AddressSanitizer and UndefinedBehaviorSanitizer, requiring a host C++17
compiler (Apple Command Line Tools suffice) and Python 3.

`BOOT_NONE` and the existing upload procedure are retained. With ARM GCC
15.3.rel1 and `-Os` throughout, this build uses 108,920 of 131,072
internal-flash bytes (83.10%). Recheck the link map after any
change. Target callback timing, physical latency, audio quality, and dropout
behavior have not been validated on hardware.

## Hardware reference

Consult the [Seed3 documentation](https://docs.daisy.audio/hardware/Seed3/)
for pinouts and electrical requirements, and
[libDaisy](https://github.com/electro-smith/libDaisy) for hardware APIs.
