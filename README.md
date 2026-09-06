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
- `libs/`: downloaded dependencies, excluded from Git.
- `build/`: generated firmware and debugging files, excluded from Git.
- `scripts/`: dependency installation and serial monitor helpers.
- `tests/`: host tests for the NAM processor boundary.
- `Makefile`: developer commands.
- `firmware.mk`: application configuration using libDaisy's build rules.

The generated compilation database and clangd cache are also excluded from Git.

## NAM integration foundation

The build compiles [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore)
at `20a04fcf466dc4233730412b120e5bbad72402c3`, the revision used by the
[TONE3000 Daisy reference](https://github.com/tone-3000/nam-pedal).
Eigen and AudioDSPTools are pinned by that revision's submodules; nlohmann JSON
is included in the Core checkout. Dependency licenses remain in `libs/`.
Run `make install` again when updating an existing workspace.

`firmware.mk` builds the engine into `build/libnam.a` using C++17,
`NAM_SAMPLE_FLOAT`, and `NAM_USE_INLINE_GEMM`. Exceptions are enabled because
upstream configuration/initialization code throws. Fast-math and approximate
activations are not enabled. The archive allows unused model families to stay
out of the linked application. Future JSON registry-based loading must explicitly
link the required model family objects so their static registrations are retained.

`src/nam_processor.h` provides `NamProcessor`, which owns an already constructed
`nam::DSP`. With audio stopped, call `Prepare(model, sample_rate, max_block_size)`
to validate mono I/O and an exact known sample rate, then reset and prewarm it.
Preparation may allocate; failure returns false and preserves the previous model.
During audio processing, `Process(input, output, frames)` accepts distinct mono
float buffers up to the prepared block size. Invalid calls return false without
writing output. The wrapper makes no processing allocations; each actual model
still needs allocation and timing validation. Preparation, destruction, and
processing must not run concurrently. Processing exceptions are not caught.

The host test uses a synthetic recurrent model to compare wrapper output with
direct upstream processing across multiple block sizes, and checks invalid
configuration and failed preparation. It needs a host C++17 compiler with
AddressSanitizer and UndefinedBehaviorSanitizer (Apple Command Line Tools suffice).

This is the engine integration only: the firmware still runs stereo soft clipping.
No amp model, model-file loader, or NAM audio routing is enabled yet. `BOOT_NONE`
is retained; the linked size with a real model and the on-board real-time budget
remain to be measured before choosing a different memory/boot arrangement.

## Hardware reference

Consult the [Seed3 documentation](https://docs.daisy.audio/hardware/Seed3/)
for pinouts and electrical requirements, and
[libDaisy](https://github.com/electro-smith/libDaisy) for hardware APIs.
