# NAM Seed3

Neural amp modeling on the Daisy Seed3 using libDaisy. Includes A2-Lite support
for Fender '65 Twin Reverb, Vox AC30 Chimey, and Marshall JCM800 (gain 5).

## Setup

On macOS, install [Homebrew](https://brew.sh) and Apple's Command Line Tools
(`xcode-select --install`), then run:

```sh
make install
bash scripts/download_models.sh
make
```

`make install` installs the ARM compiler, DFU uploader, and `jq`, and fetches pinned
dependencies. The download script extracts the existing Lite models from
Tone3000 captures into Git-ignored `models/local/`.

The models' T3K licenses permit local use but require author permission to
redistribute model data, including generated weights or firmware containing them.

## Upload and play

Connect the Seed3 with a USB-C data cable. Hold **BOOT**, press and release
**RESET**, then release **BOOT** to enter DFU mode.

```sh
make upload
make monitor
```

Firmware starts with Fender selected. Type a key in the serial monitor;
no Enter is needed:

| Key | Amp |
| --- | --- |
| `0` | Clean bypass |
| `1` | Fender '65 Twin Reverb |
| `2` | Vox AC30 Chimey |
| `3` | Marshall JCM800 G5 |

Audio runs at 48 kHz, routing the left input to both outputs. Switching amps
briefly interrupts playback. These are amp-only models; cabinet filtering
and hardware gain calibration are not implemented.

If multiple serial ports are connected, use `make monitor PORT=/dev/cu.usbmodem…`.
Exit the monitor with **Ctrl-A**, then **K**, then **Y**.

## Development

| Command | Purpose |
| --- | --- |
| `make` | Build firmware and embed all three models. |
| `make test` | Compare against upstream NAM and run sanitized host audio tests. |
| `make clean` | Remove build outputs. |
| `make format` | Format application C/C++ source. |
| `make compiledb` | Generate the clangd compilation database. |
| `make help` | List commands. |

Formatting and clangd setup require `brew install clang-format compiledb`.
Host tests require a C++20 compiler.
