# Shared firmware/host model preparation. Keep the original .nam as the source.
MODEL_HEADER = build/generated/embedded_model_data.h
NAM_LSTM_SOURCE = build/nam/NAM/lstm.cpp
NAM_LSTM_HEADER = build/nam/NAM/lstm.h

$(MODEL_HEADER): models/test_lstm.nam scripts/convert_model.py
	python3 scripts/convert_model.py $< $@

build/nam/.prepared: patches/nam-lstm.patch $(NAM_DIR)/NAM/lstm.cpp $(NAM_DIR)/NAM/lstm.h
	@mkdir -p build/nam/NAM
	cp $(NAM_DIR)/NAM/lstm.cpp $(NAM_DIR)/NAM/lstm.h build/nam/NAM/
	patch -s -d build/nam -p1 < patches/nam-lstm.patch
	touch $@

$(NAM_LSTM_SOURCE) $(NAM_LSTM_HEADER): build/nam/.prepared
	@test -f $@
