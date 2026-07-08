# SubOsc — VST3 port of the web synth

This is a real JUCE C++ plugin project that reimplements the synth engine
from the browser version: 3 oscillators → mixer → resonant lowpass filter →
ADSR envelope, plus a built-in 16-step sequencer where each step has its own
note, length, and filter-frequency knob (turning either one all the way down
silences that step).

**Important — please read before building:** I don't have a C++ compiler,
the JUCE framework, or a VST3 SDK available in the sandbox I run code in,
and I have no way to load a plugin into an actual DAW to test it. So this
code has been written carefully by hand and reviewed line-by-line, but it
has **not been compiled or run**. Treat it as a strong, complete starting
point rather than a guaranteed-perfect drop-in — you (or anyone with
Xcode/Visual Studio experience) may need to fix a small build error or two.
Everything here uses standard, well-documented JUCE APIs, so any issues
should be quick to resolve.

## What's different from the web version

- **No piano-roll grid.** JUCE's UI is built from code rather than
  HTML/CSS, so instead of clicking cells in a grid, each step has its own
  small **Note** knob (in addition to the Active toggle, Length knob, and
  Filter Freq knob). Functionally equivalent, just a knob instead of a click.
- Everything else — the 3 oscillators, mixer levels, glide, resonance,
  EG amount, ADSR, master volume, tempo, and all 16 steps' length/filter
  behavior (including "turn all the way down to silence that step") — works
  the same way it did in the browser.
- Every knob is a real, host-automatable parameter, so Reason can automate
  any of them from its own automation lanes.

## What you need to build it

- **Windows:** Visual Studio 2022 (Community edition is free) with the
  "Desktop development with C++" workload.
- **macOS:** Xcode (free from the App Store) and the Xcode command line tools.
- **CMake** 3.22 or newer (https://cmake.org/download/) on either platform.
- An internet connection the first time you build (CMake will download the
  JUCE framework automatically).

You do **not** need to buy or download anything else — JUCE is free for
this kind of use, and this project fetches it for you.

## Build steps

Open a terminal (macOS) or "Developer Command Prompt for VS 2022" (Windows)
in this folder, then run:

```bash
cmake -B build
cmake --build build --config Release
```

The first `cmake -B build` step will download JUCE (a few hundred MB) —
this can take a few minutes. Once it's done, the actual compile
(`cmake --build ...`) usually takes 2-5 minutes.

When it finishes, look for the built plugin at:

- **Windows:** `build/SubOsc_artefacts/Release/VST3/SubOsc.vst3`
- **macOS:** `build/SubOsc_artefacts/Release/VST3/SubOsc.vst3`

There's also a Standalone build (a plain desktop app version you can run
directly, no DAW needed) in the same `Release` folder, which is a handy way
to sanity-check the plugin works before loading it into Reason.

## Installing into Reason

Reason 12.5 and later supports 64-bit VST3 plugins.

1. Copy `SubOsc.vst3` into your system's VST3 folder:
   - **Windows:** `C:\Program Files\Common Files\VST3\`
   - **macOS:** `/Library/Audio/Plug-Ins/VST3/`
2. In Reason, open Preferences → Advanced (or the VST settings page) and make
   sure VST3 plugin scanning is enabled, then let it rescan.
3. SubOsc will show up on the **Instruments** device palette (and in the
   Create menu) alongside your Rack Extensions. Create one like any other
   instrument device.
4. Click the plugin's "Open" button to bring up its window, hit **Play** on
   the built-in sequencer to hear the default pattern, and start turning
   knobs.

## If the build fails

The most common first-build hiccups with any JUCE/CMake project:

- **CMake can't find a compiler** — make sure you opened the correct
  "Developer" terminal/prompt (see above) so `cl.exe` (Windows) or `clang`
  (macOS) is on your PATH.
- **Git isn't installed** — the JUCE download uses git; install Git for
  Windows or `xcode-select --install` on macOS if you don't have it yet.
- Any actual C++ compile error — copy the exact error text into a search
  engine or back to me; these are almost always a one-line fix (a missing
  include, a renamed JUCE API between versions, etc.).

## Known simplifications / things a real plugin dev might improve

- The oscillators are simple, non-band-limited waveforms (naive saw/square),
  so you may hear a bit of aliasing at high notes — the original web version
  has the same characteristic since it's also basic oscillator waveforms.
- The filter's cutoff is recalculated every single sample for accuracy,
  which is a bit more CPU-hungry than a "proper" commercial plugin might do;
  totally fine for personal use, just mentioning it.
- Voice stealing (what happens if more than 8 notes overlap) is very basic —
  it just reuses the oldest voice slot rather than doing a smooth crossfade.
