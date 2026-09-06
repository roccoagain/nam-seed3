TARGET = passthrough
NAM_DIR = libs/NeuralAmpModelerCore
NAM_SOURCES = $(addprefix $(NAM_DIR)/NAM/,activations.cpp conv1d.cpp convnet.cpp dsp.cpp get_dsp.cpp lstm.cpp ring_buffer.cpp util.cpp wavenet.cpp)
CPP_SOURCES = src/main.cpp src/nam_processor.cpp $(NAM_SOURCES)
C_INCLUDES = -I$(NAM_DIR) -I$(NAM_DIR)/Dependencies/eigen -I$(NAM_DIR)/Dependencies/nlohmann
C_DEFS = -DNAM_SAMPLE_FLOAT -DNAM_USE_INLINE_GEMM
CPP_STANDARD = -std=gnu++17
LIBDAISY_DIR = libs/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
APP_TYPE = BOOT_NONE
include $(SYSTEM_FILES_DIR)/Makefile

# NAM uses exceptions for configuration errors. Keep IEEE floating-point
# behavior until model accuracy and target performance have been measured.
CPPFLAGS += -fexceptions

# Archive the engine so unused model families and their static registration
# code are not linked into the current soft-clip application.
NAM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(NAM_SOURCES:.cpp=.o)))
OBJECTS := $(filter-out $(NAM_OBJECTS),$(OBJECTS))
NAM_LIBRARY = $(BUILD_DIR)/libnam.a
LIBS := $(NAM_LIBRARY) $(LIBS)
$(NAM_LIBRARY): $(NAM_OBJECTS)
	$(CXX:g++=ar) rcs $@ $^

# Used in dry-run mode to generate clangd commands without requiring a link.
.PHONY: compile-objects
compile-objects: $(OBJECTS) $(NAM_OBJECTS)

$(OBJECTS) $(NAM_OBJECTS): firmware.mk
$(BUILD_DIR)/$(TARGET).elf: firmware.mk $(LIBDAISY_DIR)/build/libdaisy.a $(NAM_LIBRARY)
