# SampleRealm: Reese

A synthesiser plugin focused on making Reese-style basses.

It builds on macOS and Windows as:
- VST3
- AU (macOS only)
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

## System Requirements

- **macOS**: 12 or later, Intel or Apple Silicon
- **Windows**: Windows 10 (version 1607) or later, 64-bit

## Build Requirements

- CMake 3.25+
- A C++23-capable compiler:
  - **macOS**: Xcode or its Command Line Tools (the `xcode` preset needs the full Xcode app)
  - **Windows**: Visual Studio 2022 or later with the "Desktop development with C++" workload
- Git

## Commands

Every command is the same on macOS and Windows, except the rows marked **macOS** or **Windows**.

| Task | Command |
|---|---|
| Build the plugin (Debug) | `cmake --workflow --preset debug` |
| Build the plugin (Release) | `cmake --workflow --preset release` |
| Run the unit tests | `cmake --workflow --preset unit` |
| Run the unit tests and pluginval | `cmake --workflow --preset test` |
| Run all tests against a Release build | `cmake --workflow --preset test-release` |
| Launch the Standalone app (Debug) | `cmake --build --preset run` |
| Install the plugin for your DAW: **macOS** | Automatic on every build |
| Install the plugin for your DAW: **Windows** | `cmake --build --preset install-windows`, from an administrator prompt |
| Open in an IDE for debugging: **macOS** | `cmake --preset xcode`, then open `build-xcode/Reese.xcodeproj` |
| Open in an IDE for debugging: **Windows** | Open `build-debug\Reese.sln` in Visual Studio after a Debug workflow |

`run` and `install-windows` use the Debug build, so run `cmake --workflow --preset debug` once first.

## Unit Tests

Smoke tests in `Tests/` use [Catch2](https://github.com/catchorg/Catch2) to drive the processor
directly: parameter ranges and defaults, bus layouts, state save/restore, and finite output across
block sizes and parameter settings. Run them with the `unit` or `test` workflow.

## Validating the Plugin

[pluginval](https://github.com/Tracktion/pluginval) loads the built plugin as a host would and
tests it for stability. It is built from source on demand, so there is nothing to install.

```bash
cmake --build --preset debug --target validate   # builds, then validates
```

Logs land in `build-debug/pluginval-logs/`. Strictness defaults to 10; use
`-DPLUGINVAL_STRICTNESS=5` (range 1–10) for a faster run, or `-DENABLE_PLUGINVAL=OFF` to skip
pluginval entirely.

The AU test validates the installed component in `~/Library/Audio/Plug-Ins/Components`, since macOS
resolves Audio Units through its registry rather than by path — so it needs `PLUGIN_COPY_AFTER_BUILD`
left on. Steinberg's VST3 conformance validator is off by default, as it pulls ~300MB of SDK for one
extra test; enable it with `-DPLUGINVAL_VST3_VALIDATOR=ON`.

## CI

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) builds the Release plugin on macOS and Windows
and runs the unit tests. Start it by hand from **Actions → CI → Run workflow**, or with
`gh workflow run ci.yml`; tick **Also run pluginval** (`-f pluginval=true`) to validate too. Each
run keeps `Reese-macOS.tar.gz` (VST3 and AU) and `Reese-Windows.tar.gz` (VST3) under **Artifacts**
for 30 days. They are unsigned.

## Debugging in Xcode

Generate the project with `cmake --preset xcode` and open `build-xcode/Reese.xcodeproj`, then:

1. Select your plugin target from the scheme dropdown
2. Go to **Product → Scheme → Edit Scheme** 
3. Click **Run** on the left sidebar
4. Under **Executable**, choose **Other** and navigate to executable.
5. Press **Cmd+R** to build and run with the chosen host