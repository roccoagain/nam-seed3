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
formula if missing, and downloads the pinned libDaisy revision and its submodules
into `libs/libDaisy/`. The compiler installer may request your administrator
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
| `make install` | Install build/upload tools and fetch libDaisy. |
| `make` or `make build` | Build libDaisy, then the firmware, incrementally. |
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
- `Makefile`: developer commands.
- `firmware.mk`: application configuration using libDaisy's build rules.

The generated compilation database and clangd cache are also excluded from Git.

## Hardware reference

Consult the [Seed3 documentation](https://docs.daisy.audio/hardware/Seed3/)
for pinouts and electrical requirements, and
[libDaisy](https://github.com/electro-smith/libDaisy) for hardware APIs.
