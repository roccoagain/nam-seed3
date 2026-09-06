TARGET = passthrough
NAM_DIR = libs/NeuralAmpModelerCore
NAM_SOURCES = src/nam_lstm_activations.cpp $(NAM_DIR)/NAM/dsp.cpp build/nam/NAM/lstm.cpp
CPP_SOURCES = src/main.cpp src/nam_processor.cpp src/nam_audio.cpp src/embedded_model.cpp $(NAM_SOURCES)
C_INCLUDES = -Ibuild/nam -Ibuild/generated -I$(NAM_DIR) -I$(NAM_DIR)/NAM -I$(NAM_DIR)/Dependencies/eigen -I$(NAM_DIR)/Dependencies/nlohmann
C_DEFS = -DNAM_SAMPLE_FLOAT -DNAM_USE_INLINE_GEMM -DNAM_EMBEDDED_LSTM_ONLY
CPP_STANDARD = -std=gnu++17
# Keep initialization code compact; preserve speed optimization in inference.
OPT = -Os
LIBDAISY_DIR = libs/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
APP_TYPE = BOOT_NONE
include $(SYSTEM_FILES_DIR)/Makefile

# NAM uses exceptions for configuration errors. Keep IEEE floating-point
# behavior until model accuracy and target performance have been measured.
CPPFLAGS += -fexceptions
include model.mk

# Archive the selected inference sources separately from application code.
NAM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(NAM_SOURCES:.cpp=.o)))
OBJECTS := $(filter-out $(NAM_OBJECTS),$(OBJECTS))
NAM_LIBRARY = $(BUILD_DIR)/libnam.a
LIBS := $(NAM_LIBRARY) $(LIBS)
$(NAM_LIBRARY): $(NAM_OBJECTS)
	rm -f $@
	$(CXX:g++=ar) rcs $@ $^

# Used in dry-run mode to generate clangd commands without requiring a link.
.PHONY: compile-objects
compile-objects: $(OBJECTS) $(NAM_OBJECTS)

$(OBJECTS) $(NAM_OBJECTS): firmware.mk
$(BUILD_DIR)/lstm.o: CFLAGS += -O2
$(BUILD_DIR)/lstm.o: $(NAM_LSTM_SOURCE) $(NAM_LSTM_HEADER)
$(BUILD_DIR)/embedded_model.o: $(MODEL_HEADER) $(NAM_LSTM_HEADER)
$(BUILD_DIR)/$(TARGET).elf: firmware.mk $(LIBDAISY_DIR)/build/libdaisy.a $(NAM_LIBRARY)
