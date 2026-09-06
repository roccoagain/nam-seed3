# Run from the repository root with make test.
.DEFAULT_GOAL := test
NAM_DIR = libs/NeuralAmpModelerCore
include model.mk
TEST_BIN = build/tests/nam_processor_test
SOURCES = tests/nam_processor_test.cpp src/nam_processor.cpp src/embedded_model.cpp \
  $(NAM_DIR)/NAM/dsp.cpp src/nam_lstm_activations.cpp $(NAM_LSTM_SOURCE)
FLAGS = -std=c++17 -O1 -g -DNAM_SAMPLE_FLOAT -DNAM_USE_INLINE_GEMM \
  -fsanitize=address,undefined -fno-omit-frame-pointer
INCLUDES = -Isrc -I$(NAM_DIR) -I$(NAM_DIR)/NAM \
  -I$(NAM_DIR)/Dependencies/eigen -I$(NAM_DIR)/Dependencies/nlohmann
EMBEDDED_FLAGS = -Ibuild/nam -Ibuild/generated -DNAM_EMBEDDED_LSTM_ONLY \
  -DEIGEN_RUNTIME_NO_MALLOC

include tests/a2.mk

.PHONY: test
test: $(TEST_BIN) build/tests/nam_audio_test build/tests/reference.f32 test-a2 $(MODEL_HEADER)
	./$(TEST_BIN)
	./build/tests/nam_audio_test
	python3 -B tests/test_convert_model.py
	python3 -B tests/test_convert_a2.py

$(TEST_BIN): $(SOURCES) src/nam_processor.h src/embedded_model.h tests/audio_stimulus.h tests/Makefile $(MODEL_HEADER) $(NAM_LSTM_HEADER) $(wildcard $(NAM_DIR)/NAM/*.h)
	@mkdir -p $(@D)
	$(CXX) $(FLAGS) $(EMBEDDED_FLAGS) $(INCLUDES) $(SOURCES) -o $@

build/tests/nam_audio_test: tests/nam_audio_test.cpp tests/audio_stimulus.h tests/allocation_guard.h \
  src/nam_audio.cpp src/nam_audio.h src/a2_lite.cpp src/a2_lite.h \
  src/amp_models.cpp src/amp_models.h src/nam_processor.cpp src/nam_processor.h \
  $(A2_HEADER) tests/Makefile
	@mkdir -p $(@D)
	$(CXX) $(FLAGS) $(EMBEDDED_FLAGS) $(INCLUDES) tests/nam_audio_test.cpp \
	  src/nam_audio.cpp src/amp_models.cpp src/a2_lite.cpp src/nam_processor.cpp $(NAM_DIR)/NAM/dsp.cpp -o $@

REFERENCE_SOURCES = tests/render_reference.cpp $(addprefix $(NAM_DIR)/NAM/,get_dsp.cpp dsp.cpp activations.cpp lstm.cpp)
build/tests/render_reference: $(REFERENCE_SOURCES) tests/audio_stimulus.h tests/Makefile
	@mkdir -p $(@D)
	$(CXX) $(FLAGS) $(INCLUDES) $(REFERENCE_SOURCES) -o $@

build/tests/reference.f32: build/tests/render_reference models/test_lstm.nam
	./build/tests/render_reference
