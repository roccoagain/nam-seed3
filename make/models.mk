# Shared firmware/host model preparation. Keep the original .nam as the source.
# Order matches AmpId and the MODELS table in scripts/convert_a2.py.
A2_NAMES = fender-twin65-a2-lite vox-ac30-chimey-a2-lite marshall-jcm800-g5-a2-lite
A2_MODELS = $(addprefix models/local/,$(addsuffix .nam,$(A2_NAMES)))
A2_HEADER = build/generated/embedded_a2_data.h

$(A2_MODELS):
	@echo 'Missing required amp models. Run bash scripts/download_models.sh first.' >&2
	@exit 1

$(A2_HEADER): $(A2_MODELS) scripts/convert_a2.py
	python3 scripts/convert_a2.py $@
