import unittest
from pathlib import Path

ROOT = Path(__file__).parents[1]
PROCESSOR = ROOT / "Source" / "PluginProcessor.cpp"
HEADER = ROOT / "Source" / "PluginProcessor.h"
CMAKE = ROOT / "CMakeLists.txt"


class LoFiAlphaContractTest(unittest.TestCase):
    def test_six_knob_parameter_contract(self):
        source = PROCESSOR.read_text()
        for parameter in ("age", "warp", "dust", "heat", "mix", "output"):
            self.assertIn(f'"{parameter}"', source)
        self.assertNotIn('"warmth"', source)
        self.assertNotIn('"wobble"', source)

    def test_real_pitch_warp_uses_fractional_delay(self):
        source = PROCESSOR.read_text()
        header = HEADER.read_text()
        self.assertIn("readWarpDelay", source)
        self.assertIn("warpDelay", header)
        self.assertIn("cubic", source.lower())

    def test_heat_has_oversampling_and_parameter_smoothing(self):
        header = HEADER.read_text()
        self.assertIn("Oversampling", header)
        self.assertIn("SmoothedValue", header)

    def test_custom_editor_is_compiled(self):
        cmake = CMAKE.read_text()
        source = PROCESSOR.read_text()
        self.assertIn("Source/PluginEditor.cpp", cmake)
        self.assertIn("DustBoxLoFiAudioProcessorEditor", source)

    def test_compact_knobs_keep_square_rotary_geometry(self):
        editor = (ROOT / "Source" / "PluginEditor.cpp").read_text()
        self.assertNotIn("110, 92", editor)
        self.assertIn("110, 132", editor)

    def test_age_reaches_obvious_old_media_range(self):
        source = PROCESSOR.read_text()
        header = HEADER.read_text()
        self.assertIn("1100.0f", source)
        self.assertIn("ageHoldCounter", header)
        self.assertIn("ageHeldSample", header)


if __name__ == "__main__":
    unittest.main()
