import unittest
from pathlib import Path


class AudioReaderOwnershipTest(unittest.TestCase):
    def test_embedded_audio_stream_is_heap_owned_by_reader(self):
        source = (Path(__file__).parents[1] / "Source" / "PluginProcessor.cpp").read_text()

        self.assertNotIn(
            "juce::MemoryInputStream input",
            source,
            "AudioFormatReader owns and deletes its InputStream on successful creation; "
            "passing a stack MemoryInputStream causes an invalid free in the reader destructor",
        )
        self.assertIn("new juce::MemoryInputStream", source)


if __name__ == "__main__":
    unittest.main()
