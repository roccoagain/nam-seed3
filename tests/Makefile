# Run from the repository root with make test.
.DEFAULT_GOAL := test
NAM_DIR = libs/NeuralAmpModelerCore
include make/models.mk
TEST_BIN = build/tests/nam_processor_test
SOURCES = tests/nam_processor_test.cpp src/nam_processor.cpp $(NAM_DIR)/NAM/dsp.cpp
FLAGS = -std=c++17 -O1 -g -DNAM_SAMPLE_FLOAT -DNAM_USE_INLINE_GEMM \
  -fsanitize=address,undefined -fno-omit-frame-pointer
INCLUDES = -Isrc -Itests -I$(NAM_DIR) -I$(NAM_DIR)/NAM \
  -I$(NAM_DIR)/Dependencies/eigen -I$(NAM_DIR)/Dependencies/nlohmann
include tests/a2.mk

.PHONY: test
test: $(TEST_BIN) build/tests/nam_audio_test test-a2
	./$(TEST_BIN)
	./build/tests/nam_audio_test
	python3 -B tests/test_convert_a2.py

$(TEST_BIN): $(SOURCES) src/nam_processor.h tests/audio_stimulus.h tests/Makefile $(wildcard $(NAM_DIR)/NAM/*.h)
	@mkdir -p $(@D)
	$(CXX) $(FLAGS) -Ibuild/generated $(INCLUDES) $(SOURCES) -o $@

build/tests/nam_audio_test: tests/nam_audio_test.cpp tests/audio_stimulus.h tests/allocation_guard.h \
  src/nam_audio.cpp src/nam_audio.h src/a2_lite.cpp src/a2_lite.h \
  src/amp_models.cpp src/amp_models.h src/nam_processor.cpp src/nam_processor.h \
  $(A2_HEADER) tests/Makefile
	@mkdir -p $(@D)
	$(CXX) $(FLAGS) -Ibuild/generated $(INCLUDES) tests/nam_audio_test.cpp \
	  src/nam_audio.cpp src/amp_models.cpp src/a2_lite.cpp src/nam_processor.cpp $(NAM_DIR)/NAM/dsp.cpp -o $@
