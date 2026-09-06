# Seed3 experiments

A C++ workspace for experimenting with audio processing on the Daisy Seed3,
using libDaisy. Firmware behavior will change as experiments develop; see
`src/main.cpp` for the current implementation.

## Setup (macOS)

Install [Homebrew](https://brew.sh) and Apple's Command Line Tools
(`xcode-select --install`), then run from the project root:

```sh
make install
bash scripts/download_models.sh
make
```

`make install` installs the Homebrew `gcc-arm-embedded` cask and `dfu-util`
formula if missing, and downloads pinned libDaisy and NeuralAmpModelerCore
revisions and their submodules into `libs/`, including a separate A2 reference
checkout for host tests. The compiler installer may request your administrator
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
| `make model` | Generate embedded weights for the three downloaded A2-Lite amps. |
| `make test` | Run upstream A2 comparisons and host audio tests with sanitizers. |
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

The required amps are Fender '65 Twin Reverb, Vox AC30 (Chimey), and Marshall
JCM800 (gain 5). Download their official A2 captures and extract the Lite models:

```sh
bash scripts/download_models.sh
```

The script requires `curl`, `jq`, and `shasum`; install `jq` with Homebrew if
missing. Files remain under Git-ignored `models/local/`. Their T3K licenses
permit local use, but redistributing the model data requires author permission.
Do not publish generated weight headers or firmware containing these weights
without that permission. The small MIT test LSTM remains for host regressions.

Firmware defaults to Fender and processes the left input to both outputs at
48 kHz with 48-sample blocks. In `make monitor`, type:

| Key | Selection |
| --- | --- |
| `0` | Clean bypass |
| `1` | Fender Twin65 |
| `2` | Vox AC30 Chimey |
| `3` | Marshall JCM800 G5 |

No Enter is needed. USB reception only queues the latest command. The main
loop stops audio before constructing and prewarming a new model, then restarts
it; switching briefly interrupts playback. Initialization failure falls back
to clean bypass. The bypass path continues advancing the loaded model state.

Input gain is 1 and output gain is 0.8; constants are in `src/nam_audio.h`.
Output is clamped to [-1, 1], and nonfinite samples are silenced. These are
amp-only captures, without a cabinet response. Hardware input/output gain
calibration and cabinet filtering are not implemented.

Once per second, the log reports the selected amp, readiness, bypass state,
callback count, maximum processing time in microseconds, and the number of
callbacks taking at least 1,000 microseconds. This measures callback processing
time, not physical latency or every possible DMA scheduling failure.

## NAM build and validation

`make build` converts all three Lite models into float32 arrays and builds the
fixed A2-Lite engine in `src/a2_lite.cpp`. The converter rejects configurations
outside the supported mono, three-channel, 23-layer WaveNet architecture.
There is no runtime JSON parser, filesystem access, or model download. The
engine reads weights directly from flash and allocates convolution history
before audio starts. It does no allocation while processing.

The existing `NamProcessor` and NAM Core DSP interface remain at
`20a04fcf466dc4233730412b120e5bbad72402c3`. Host reference tests use a separate,
unmodified Core checkout at `2563c0fd4cb1f9ce457d89a761738ea15097e1f3`, with its
generic WaveNet implementation (the upstream A2 fast path is disabled).
Run `make install` to fetch both pinned dependencies. The firmware uses C++17;
the A2 reference executable requires a host C++20 compiler.

`make test` compares every amp against upstream output for silence, impulse,
step, and multitone input across different block sizes. It also checks amp
switching, reset, bypass, stereo duplication, output bounds, conversion
rejection, and absence of C++ processing allocations under AddressSanitizer
and UndefinedBehaviorSanitizer. The retained LSTM tests use the adaptation in
`patches/nam-lstm.patch`; it is no longer part of firmware inference.

All three Lite weight arrays total 22,452 bytes. Each active model allocates
112,416 bytes of layer history plus its head history and bookkeeping; switching
briefly holds both old and new models until preparation succeeds. Static linker
RAM totals do not include this heap allocation.

`BOOT_NONE` and the existing upload procedure are retained. All three models
fit in internal flash with the focused engine, so external flash is not needed.
With ARM GCC 15.3.rel1 and `-Os`, the clean A2 build uses 118,328 of 131,072
bytes (90.28%). Recheck the link map after changes. No fast-math is enabled.
Target callback timing, switching transients, physical latency, and audio quality
still require hardware validation; a host test or ARM build is not proof of
real-time operation.

## Hardware reference

Consult the [Seed3 documentation](https://docs.daisy.audio/hardware/Seed3/)
for pinouts and electrical requirements, and
[libDaisy](https://github.com/electro-smith/libDaisy) for hardware APIs.
