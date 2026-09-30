# SampleRealm: Reese

A synthesiser plugin focused on making Reese-style basses.

It builds as:
- VST3
- AU
- Standalone
## Sound Design Direction

The synth is designed as a compact Reese bass instrument.

The current voice architecture is:

- Saw/Triangle/Square oscillator A
- Saw/Triangle/Square  oscillator B 
- Detune
- Sub sine oscillator one octave down
- Amp envelope
- Low-pass filter
- Tempo-syncable LFO modulation on filter cutoff
- Drive
- Output gain

This gives you a dark, moving, detuned bass sound that works as a solid Reese starting point.

## Controls

The UI currently exposes 9 controls:

- Output — final output level in dB
- Detune — pitch spread between the two saw oscillators
- Sub — sub oscillator level
- Cutoff — low-pass filter cutoff frequency
- Resonance — filter resonance
- Drive — saturation amount
- LFO Rate — speed of filter movement (Hz or tempo-synced note divisions)
- LFO Depth — strength of filter movement

## Build Requirements

- CMake 3.25+
- A C++23-capable compiler
- Git
- macOS development environment for AU/Standalone/VST3 builds

## Building

### Debug

```bash
cmake --preset debug
cmake --build --preset debug
```

### Release

```bash
cmake --preset release
cmake --build --preset release
```

## Validating the Plugin

[pluginval](https://github.com/Tracktion/pluginval) loads the built plugin as a host would and
tests it for stability. It is built from source on demand, so there is nothing to install.

```bash
cmake --build --preset debug --target validate   # builds, then validates
ctest --preset debug                             # validates an existing build
```

Logs land in `build-debug/pluginval-logs/`. Strictness defaults to 10; use
`-DPLUGINVAL_STRICTNESS=5` (range 1–10) for a faster run, or `-DENABLE_PLUGINVAL=OFF` to skip
pluginval entirely.

The AU test validates the installed component in `~/Library/Audio/Plug-Ins/Components`, since macOS
resolves Audio Units through its registry rather than by path — so it needs `COPY_PLUGIN_AFTER_BUILD`
left on. Steinberg's VST3 conformance validator is off by default, as it pulls ~300MB of SDK for one
extra test; enable it with `-DPLUGINVAL_VST3_VALIDATOR=ON`.

## Debugging in Xcode

To debug the plugin in Xcode with an executable:

### 1. Generate Xcode Project

```bash
cmake -B build-xcode -G Xcode
open build-xcode/Reese.xcodeproj
```

### 2. Configure Debugging

1. Select your plugin target from the scheme dropdown
2. Go to **Product → Scheme → Edit Scheme** 
3. Click **Run** on the left sidebar
4. Under **Executable**, choose **Other** and navigate to executable.

### 3. Build and Run

1. Press **Cmd+B** to build the plugin
2. Press **Cmd+R** to run with AudioPluginHost
4. Load your plugin in AudioPluginHost