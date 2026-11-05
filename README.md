# SampleRealm: Reese

A synthesiser plugin focused on making Reese-style basses.

It builds on macOS and Windows as:
- VST3
- AU (macOS only)
- Standalone
## Sound Design Direction

A compact, monophonic Reese bass instrument. Notes play two octaves below the key.

- Oscillators A and B: square, saw or triangle, detuned against each other, with up to 8 unison voices
- Sub sine oscillator one octave below oscillator A
- Drive (tanh saturation)
- Low-pass filter, 12 or 24 dB, with a sine LFO on its cutoff, free or synced to the host tempo
- Amp envelope
- Mono, last-note priority: a note played while another is held glides to it (Glide) without
  restarting the envelope, and releasing it returns to the note still held

## Controls

| Control | What it does |
|---|---|
| Osc A, Osc B | Waveform: square, saw or triangle |
| Sub | Sub oscillator level |
| Detune | Spread between oscillators A and B and across the unison voices, in cents |
| Glide | Time to glide between held notes, in seconds |
| Unison | Voices per oscillator, 1 to 8 |
| Drive | Saturation amount |
| Output | Output level, as linear gain from 0 to 1 |
| Cutoff, with 12dB / 24dB | Low-pass cutoff in Hz, and its slope |
| Resonance | Filter resonance |
| LFO Rate, with Sync | LFO speed in Hz, or a note length synced to the host's tempo (174 BPM when it has none) |
| LFO Depth | How far the LFO moves the cutoff |
| Attack, Decay, Sustain, Release | The amp envelope |

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
| Build the plugin with the WebView editor (Release) | `cmake --workflow --preset webview` |
| Launch the Standalone app (Debug) | `cmake --build --preset run` |
| Install the plugin for your DAW: **macOS** | Automatic on every build |
| Install the plugin for your DAW: **Windows** | `cmake --build --preset install-windows`, from an administrator prompt |
| Open in an IDE for debugging: **macOS** | `cmake --preset xcode`, then open `build-xcode/Reese.xcodeproj` |
| Open in an IDE for debugging: **Windows** | Open `build-debug\Reese.sln` in Visual Studio after a Debug workflow |

`run` and `install-windows` use the Debug build, so run `cmake --workflow --preset debug` once first.

## Unit Tests

Tests in `Tests/` use [Catch2](https://github.com/catchorg/Catch2) to drive the processor directly:
the parameter snapshot hosts and saved sessions rely on, bus layouts, state save/restore, and finite
output across block sizes and parameter settings. Run them with the `unit` or `test` workflow.
How notes play is tested on the wasm engine, in `wasm/engine.test.mjs`.

## Web Demo and WebView Editor

The DSP in `Source/dsp/` also builds to WebAssembly for a playable web demo, with a React copy of the
editor in `webui/`. Needs Emscripten (`brew install emscripten`) and Node 24+.

| Task | Command |
|---|---|
| Build and test the demo (engine and UI tests) | `cd wasm && cmake --workflow --preset wasm` |
| Serve the demo with live reload at http://localhost:8099 | `cd webui && npm run serve` |
| Typecheck and test the UI on their own | `cd webui && npm run typecheck && npm test` |
| Build the plugin with the WebView editor | `cmake --workflow --preset webview` |

- The demo builds to `wasm/build-wasm/<company>-<project>-<version>/`, named from `COMPANY_NAME`
  (one word), `project()` and `VERSION` in `CMakeLists.txt`. Upload that folder; the site embeds its
  `index.html` in an iframe. [`wasm.yml`](.github/workflows/wasm.yml) builds the same folder on
  GitHub as an artifact of that name, run by hand.
- The demo plays from the computer keyboard (A to '), the on-screen keys or a MIDI keyboard,
  at 174 BPM. Its presets stay in the browser.
- The `webview` build installs over the native one; run the `release` workflow to switch back.
- Delete `wasm/patches/juce-wasm.patch` once JUCE fixes its wasm support.

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
for 30 days. They are unsigned. [`wasm.yml`](.github/workflows/wasm.yml) builds and tests the web
demo the same way: **Actions → Web demo → Run workflow**, or `gh workflow run wasm.yml`.

## Debugging in Xcode

Generate the project with `cmake --preset xcode` and open `build-xcode/Reese.xcodeproj`, then:

1. Select your plugin target from the scheme dropdown
2. Go to **Product → Scheme → Edit Scheme** 
3. Click **Run** on the left sidebar
4. Under **Executable**, choose **Other** and navigate to executable.
5. Press **Cmd+R** to build and run with the chosen host