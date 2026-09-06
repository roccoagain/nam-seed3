TARGET = passthrough
CPP_SOURCES = src/main.cpp
LIBDAISY_DIR = libs/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
APP_TYPE = BOOT_NONE
include $(SYSTEM_FILES_DIR)/Makefile

# Used in dry-run mode to generate clangd commands without requiring a link.
.PHONY: compile-objects
compile-objects: $(OBJECTS)

$(OBJECTS): firmware.mk
$(BUILD_DIR)/$(TARGET).elf: firmware.mk $(LIBDAISY_DIR)/build/libdaisy.a
