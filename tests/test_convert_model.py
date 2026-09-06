import importlib.util
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("convert_model", "scripts/convert_model.py")
converter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(converter)


class ConvertModelTest(unittest.TestCase):
    def test_source_checksum(self):
        self.assertEqual(hashlib.sha256(Path("models/test_lstm.nam").read_bytes()).hexdigest(),
                         "df9f78c49f49c2bb32411df47e3f53746075adb206b92d017e06379d1e56234a")

    def test_reproducible(self):
        self.assertEqual(converter.convert("models/test_lstm.nam"),
                         Path("build/generated/embedded_model_data.h").read_text())

    def test_rejects_unsupported_and_corrupt_models(self):
        original = json.loads(Path("models/test_lstm.nam").read_text())
        cases = [("version", "9.0.0"), ("architecture", "WaveNet"),
                 ("sample_rate", 44100), ("weights", [0.0]),
                 ("weights", [float("nan")] * 70)]
        for key, value in cases:
            model = dict(original, **{key: value})
            self.reject(model)
        for key, value in [("hidden_size", 100), ("num_layers", 2),
                           ("input_size", 2), ("out_channels", 2),
                           ("unknown_feature", True)]:
            model = dict(original, config=dict(original["config"], **{key: value}))
            self.reject(model)

    def reject(self, model):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.nam"
            path.write_text(json.dumps(model))
            with self.assertRaises(ValueError):
                converter.convert(path)


if __name__ == "__main__":
    unittest.main()
