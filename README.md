# Seed3 48 kHz passthrough

Stereo passthrough using libDaisy, 48 kHz and 48 samples per callback.
Channel 1 copies physical pin 16 to pin 18; channel 2 copies pin 17 to pin 19.
Connect AGND (20) to DGND (40). Use a small line-level test signal and a
scope or appropriate line input; guitar and headphone interfaces need circuitry.

## Setup (macOS)

Requires [Homebrew](https://brew.sh) and Apple's Command Line Tools
(`xcode-select --install`). Run from this directory:

```sh
make install
make build
```

`make install` installs the `gcc-arm-embedded` cask and `dfu-util` formula
if missing, then fetches libDaisy at the commit pinned in `scripts/install.sh`,
including its submodules. Homebrew handles libusb automatically. The compiler
cask may request administrator authentication through the macOS installer.
Existing Homebrew installations are reused, not automatically upgraded.
The installer does not overwrite a libDaisy checkout at another revision.

Build and upload tools are resolved through your shell's `PATH`; there is no
project-local toolchain. Only libDaisy is kept under `libs/libDaisy/`.
Plain `make` builds libDaisy first, then the application, incrementally.
After changing compiler versions, run `make clean` before rebuilding.

## Upload

Connect a USB-C **data** cable. Hold BOOT, press and release RESET, then release
BOOT. With only the intended DFU device connected, run:

```sh
make upload
```

`make program-dfu` is an alias. Upload writes the application to internal flash
at `0x08000000`, replacing the current firmware. No separate Daisy bootloader
or ST-Link is required for this small application. If needed, press RESET after
upload. USB DFU enumeration alone does not validate the audio path.

## USB serial diagnostics

Firmware enables libDaisy USB CDC logging on the USB-C port without waiting
for a terminal. After flashing/resetting (outside DFU mode), run in your terminal:

```sh
make monitor
```

This opens the single `/dev/cu.usbmodem*` device in `screen` at 115200.
If multiple ports exist, it lists them instead of guessing; select one with
`make monitor PORT=/dev/cu.usbmodemYOUR_PORT`. Exit screen with Ctrl-A,
then K, then Y. A serial monitor such as Arduino's also works.
The startup message may appear before you connect, so a status line repeats
about once per second with uptime, sample rate, block size, and the number
of audio callbacks since the previous report (normally around 1000).
The onboard LED toggles each report. Logging happens only in the main loop.
This verifies firmware activity and audio callback execution without a carrier;
it does not verify analog input/output quality. Unconnected audio inputs may float.

Output: `build/passthrough.bin` (also `.elf`, `.hex`, and linker map).
`make clean` removes application and libDaisy build outputs, retaining library source and Homebrew tools.
`make JOBS=8` changes libDaisy build concurrency.

References: [Seed3 docs](https://docs.daisy.audio/hardware/Seed3/),
[Daisy C++ setup](https://docs.daisy.audio/tutorials/cpp-dev-env/),
[libDaisy](https://github.com/electro-smith/libDaisy).
