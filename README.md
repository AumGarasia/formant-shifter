# Formant Shifter

A pitch and formant shifter plugin (AU and VST3, macOS), modeled on the idea of Soundtoys'
AlterBoy: shift the pitch and the formants of a voice independently.

## Controls

| Control | Range | Notes |
|---|---|---|
| Pitch | ±24 semitones | |
| Formant | ±12 semitones | Independent of pitch. With Link on, it is an offset on top of the pitch shift. |
| Link | on / off | Formants follow the pitch ("resampling" sound). The total is clamped to ±24 st. |
| Mix | 0–100 % | Dry path is delayed to match the wet path exactly. |
| Output | −24 to +12 dB | |

Reported latency is 80 ms (3528 samples at 44.1 kHz), chosen from measurements and listening.

## Requirements

- macOS 12 or later, Apple Silicon or Intel (builds a universal binary)
- Xcode and CMake 3.22 or later
- Ableton Live or any host for AU / VST3

## Getting the source

```bash
git clone <repository-url> formant-plugin
cd formant-plugin
git submodule update --init
```

Do **not** add `--recursive`: Signalsmith Stretch contains a nested submodule that is only
used by its own demo tool.

Pinned submodules (`external/`): JUCE 8.0.12, Signalsmith Stretch 1.4.0, Signalsmith Linear
0.6.4, Catch2.

## Building

| Command | What it does |
|---|---|
| `./build.sh` | Debug build of the DSP library, tests and tools, then runs the test suite |
| `./build-plugin.sh` | Release build of the AU and VST3, installs them to `~/Library/Audio/Plug-Ins`, runs `auval` |
| `./validate-plugin.sh [level]` | Runs pluginval (install it separately) on both formats; default level 5 |

Always test the plugin with the Release build: the DSP is several times slower in Debug.
After rebuilding, quit and reopen the host, which keeps the old binary loaded.

## Layout

```
src/dsp/         JUCE-free DSP: engine interface, Signalsmith engine, EffectCore (mix, gain,
                 dry path, chunking), ramps, formant routing, offline renderer
src/ui/          look-and-feel, seven-segment drawing, value formatting
src/             plugin processor, editor, parameters
tests/           Catch2 tests and fixtures
tools/offline/   offline_render: run a WAV through an engine (pitch, formant, block length)
tools/probe/     engine_probe: measure pitch/formant behaviour on a synthetic vowel
external/        pinned third-party submodules
```

The plugin is thin glue around `fsdsp::EffectCore`; all audio logic lives in `src/dsp` and is
tested without any plugin framework.

## Tools

```bash
build/offline_render_artefacts/Release/offline_render in.wav --engine signalsmith@80 --pitch 5 --formant -2
build/Release/engine_probe formants        # are pitch and formant independent?
build/Release/engine_probe blocks -7       # latency / accuracy / purity per block length
```

## Testing

`./build.sh` runs unit tests for the engine contract, latency alignment, the dry/wet path
(a mix of 0.5 against an inverted engine must be exact silence), parameter ramps, formant
independence (measured on a synthetic vowel), and real-time safety (no heap allocation while
processing, counted through a replaced `operator new`; this does not see raw `malloc` or locks).

Plugin-level checks: `auval`, pluginval (levels 5 and 10), and the cancellation test in a DAW
(same clip on two tracks, one phase-inverted, plugin at Mix 0 %: the sum must be silence).

## Known limitations

- macOS only.
- The VST3 has not been checked with Steinberg's validator.
- Alterboy's Robot and Quantize modes (pitch-tracked) are not implemented.

## Licensing

See `THIRD_PARTY_NOTICES.md`. Check the JUCE licence terms before distributing binaries.