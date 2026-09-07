#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
root="$PWD"
daisy_commit=cc146d5065dd8286078a662e2830bf820c37a612
nam_commit=2563c0fd4cb1f9ce457d89a761738ea15097e1f3

command -v brew >/dev/null || {
    echo 'Install Homebrew first: https://brew.sh' >&2
    exit 1
}
# Keep already-installed versions; upgrading tools is a separate decision.
if ! brew list --cask gcc-arm-embedded >/dev/null 2>&1; then
    brew install --cask gcc-arm-embedded
fi
for formula in dfu-util jq; do
    if ! brew list --formula "$formula" >/dev/null 2>&1; then
        brew install "$formula"
    fi
done
for tool in git make arm-none-eabi-g++ dfu-util jq; do
    command -v "$tool" >/dev/null || {
        echo "Missing $tool on PATH; check your Homebrew shell setup." >&2
        exit 1
    }
done
mkdir -p "$root/libs"

if [[ ! -d "$root/libs/libDaisy" ]]; then
    git clone --no-checkout https://github.com/electro-smith/libDaisy.git "$root/libs/libDaisy"
    git -C "$root/libs/libDaisy" checkout --detach "$daisy_commit"
fi
if [[ $(git -C "$root/libs/libDaisy" rev-parse HEAD) != "$daisy_commit" ]]; then
    echo "libs/libDaisy differs from pinned revision $daisy_commit; leaving it untouched." >&2
    exit 1
fi
git -C "$root/libs/libDaisy" submodule update --init --recursive

if [[ ! -d "$root/libs/NeuralAmpModelerCore" ]]; then
    git clone --no-checkout https://github.com/sdatkinson/NeuralAmpModelerCore.git "$root/libs/NeuralAmpModelerCore"
    git -C "$root/libs/NeuralAmpModelerCore" checkout --detach "$nam_commit"
fi
if [[ $(git -C "$root/libs/NeuralAmpModelerCore" rev-parse HEAD) != "$nam_commit" ]]; then
    echo "libs/NeuralAmpModelerCore differs from pinned revision $nam_commit; leaving it untouched." >&2
    exit 1
fi
git -C "$root/libs/NeuralAmpModelerCore" submodule update --init --recursive

arm-none-eabi-g++ --version
dfu-util --version
echo 'Dependencies ready. Run bash scripts/download_models.sh, then make build.'
