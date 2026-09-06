# Run from the repository root with make test.
NAM_DIR = libs/NeuralAmpModelerCore
TEST_BIN = build/tests/nam_processor_test
SOURCES = tests/nam_processor_test.cpp src/nam_processor.cpp \
  $(NAM_DIR)/NAM/dsp.cpp $(NAM_DIR)/NAM/activations.cpp $(NAM_DIR)/NAM/lstm.cpp

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(SOURCES) src/nam_processor.h tests/Makefile $(wildcard $(NAM_DIR)/NAM/*.h)
	@mkdir -p $(@D)
	$(CXX) -std=c++17 -O1 -g -DNAM_SAMPLE_FLOAT -DNAM_USE_INLINE_GEMM \
	  -fsanitize=address,undefined -fno-omit-frame-pointer \
	  -Isrc -I$(NAM_DIR) -I$(NAM_DIR)/Dependencies/eigen \
	  -I$(NAM_DIR)/Dependencies/nlohmann $(SOURCES) -o $@
