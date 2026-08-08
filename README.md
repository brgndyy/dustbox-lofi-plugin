# DustBox LoFi

DustBox LoFi is an experimental six-control lo-fi color VST3 plug-in built with C++17 and JUCE.

## Controls

- `AGE` — bandwidth and time-resolution degradation
- `WARP` — wow, flutter, and random pitch drift
- `DUST` — surface noise, clicks, and pops
- `HEAT` — saturation and harmonic color
- `MIX` — dry/wet balance
- `OUTPUT` — final output level

## Status

Alpha prototype (`0.2.1`). The current build target is macOS Apple Silicon (`arm64`). Intel, universal binary, notarized distribution, and broad DAW compatibility are not yet guaranteed.

## Checkout

```bash
git clone --recurse-submodules https://github.com/brgndyy/dustbox-lofi-plugin.git
cd dustbox-lofi-plugin
```

For an existing checkout:

```bash
git submodule update --init --recursive
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target DustBoxLoFi_VST3
```

The VST3 bundle is generated under:

```text
build/DustBoxLoFi_artefacts/Release/VST3/DustBox LoFi.vst3
```

## Tests

```bash
python3 -m unittest discover -s tests -p 'test_*.py'
cmake --build build --config Release --target DustBoxDSPTest
./build/DustBoxDSPTest_artefacts/Release/DustBoxDSPTest
```

## Repository contents

- `Source/` — processor and editor implementation
- `assets/` — embedded audio source assets
- `tests/` — regression and DSP checks
- `docs/` — development articles and notes
- `JUCE/` — pinned JUCE submodule

No public release or stable API is promised at the alpha stage.
