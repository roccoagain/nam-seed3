.DEFAULT_GOAL := build
JOBS ?= 4

.PHONY: install build upload program-dfu clean help
install:
	bash scripts/install.sh

build:
	@command -v arm-none-eabi-g++ >/dev/null || { echo 'Missing ARM compiler; run make install.'; exit 1; }
	@test -f libs/libDaisy/core/Makefile || { echo 'Run make install first.'; exit 1; }
	$(MAKE) -C libs/libDaisy -j$(JOBS)
	$(MAKE) -f firmware.mk all

upload: build
	@command -v dfu-util >/dev/null || { echo 'Run make install first.'; exit 1; }
	$(MAKE) -f firmware.mk program-dfu

program-dfu: upload

clean:
	rm -rf build
	@if test -f libs/libDaisy/Makefile; then $(MAKE) -C libs/libDaisy clean; fi

help:
	@echo 'make install  Install Homebrew compiler/uploader and fetch pinned libDaisy.'
	@echo 'make build    Build libDaisy and 48 kHz passthrough firmware.'
	@echo 'make upload   Build and flash via USB; enter BOOT + RESET mode first.'
	@echo 'make clean    Remove firmware and libDaisy build outputs.'
