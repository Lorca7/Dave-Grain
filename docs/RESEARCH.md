# Granular synthesis research → Dave Grain design

Granular synthesis (Xenakis' "grains of sound", formalised by Curtis Roads in *Microsound* and
made real-time by Barry Truax) builds sound out of many tiny, overlapping, windowed snippets
("grains") of a source sample. Every granular instrument is a different interface onto the same
handful of ideas. Below is what the well-known instruments do, the fundamental principles they
share, and where each one lives in Dave Grain.

## Instruments surveyed

| Instrument | Notable ideas |
|---|---|
| **Ableton Granulator II / III** (Robert Henke) | Built for Live. File position + scan, grain size, spray, pitch, FM; Granulator III adds Classic/Loop/Cloud modes and MPE. Drag-and-drop from Live's browser onto the waveform. |
| **Mutable Instruments Clouds / Beads** | Iconic minimal control set: Position, Size, Pitch, Density, Texture (window shape), Freeze, Blend (dry/wet, spread, feedback, reverb). Density knob crosses from regular to random grain timing. |
| **Logic Alchemy** (granular mode) | Grain size, density, position randomisation, window shape, independent time (speed) and pitch. |
| **Steinberg Padshop 2** | Grain duration, density, position + position spread, pitch randomisation, grain window, stereo spread; pad-oriented with filters, LFOs and FX. |
| **Spectrasonics Omnisphere** (granular) | Granular applied to any soundsource; intensity/depth/speed/spread — granular as a texture "amount" rather than a separate synth. |
| **Arturia Pigments** (sample engine, granular) / **Efx Fragments** | Grain engine with position/spray, size, density, pitch quantise to scales, reverse probability, and heavy modulation; visual feedback of grains over the waveform. |
| **Output Portal** | Granular FX with macros and big visual feedback; a strong "randomise / happy accident" workflow. |
| **Audio Damage Quanta** | Two grain engines, scan/position, size, density, pitch spray, envelope shapes; deep modulation. |
| **Borderlands Granular** (Chris Carlson) | Purely visual: clouds placed over the waveform, grains drawn as they play — very intuitive and inspiring. |

## Fundamental principles (and how Dave Grain implements them)

| Principle | Why it matters | In Dave Grain |
|---|---|---|
| **Grain size** | Short (< 30 ms) grains become buzzy/tonal textures; long (> 100 ms) grains keep the source recognisable. | `Size` 5 ms – 2 s, `Size Var` randomises per grain. |
| **Grain window / envelope** | Each grain is faded in/out to avoid clicks; the shape defines the character (smooth vs. punchy). | `Window Shape`: Hann, Gaussian, Triangle, Tukey, Percussive, Swell — with live preview. |
| **Density** | Grains per second; with size it sets overlap (thin & sparkly vs. thick cloud). | `Density` 0.5 – 200 /s per voice, with automatic overlap gain-compensation so dense clouds don't clip. |
| **Synchronous vs asynchronous** | Evenly spaced grains form a pitched "pulse" (pitch-synchronous granular); random spacing forms a cloud. | `Jitter` blends a regular clock into Poisson (exponential) spacing. |
| **Position** | Where in the source grains are read. | `Position` knob, or click/drag on the waveform. |
| **Spray / position randomisation** | Smears the read point for diffuse textures. | `Spray` (shaded on the waveform), scroll-wheel on the waveform. |
| **Scan / time-stretch** | Moving the read point independently of pitch = time-stretching, reverse playback or a frozen moment. | `Scan` −200 % … +200 % per voice, `Freeze` button. |
| **Pitch & transposition** | Grain playback rate sets pitch independently of time. | `Pitch` ±24 st, `Fine`, MIDI key tracking around a `Root` note, pitch-bend ±2 st. |
| **Pitch randomisation & quantisation** | Random per-grain pitch gives chorus/shimmer; quantising it to a scale keeps it musical. | `Random` ±24 st, `Scale`: Semitones, Octaves, Fifths, Major, Minor, Pentatonic. |
| **Reverse grains** | Classic granular colour. | `Reverse` probability per grain. |
| **Stereo spread** | Each grain gets its own pan for a wide, immersive cloud. | `Spread` (equal-power per-grain panning). |
| **Polyphony / playability** | Make it an instrument, not just an effect. | 8-voice polyphony with voice stealing, ADSR, velocity, Hold (drone latch) + sustain pedal. |
| **Modulation** | Granular comes alive when parameters move. | 2 LFOs (Sine, Triangle, Saw, Square, Random S&H, smooth Drift), free or tempo-synced to Live, targeting Position, Spray, Size, Density, Pitch, Cutoff, Spread or Volume. |
| **Post-processing** | Filters and space glue the cloud together. | State-variable filter (LP/BP/HP), ping-pong delay, reverb, soft-clipped output. |
| **Visual feedback** | Seeing grains (Borderlands, Pigments, Portal) makes the abstract parameters intuitive. | Animated waveform: every live grain is drawn as a glowing particle (x = position, y = pan), plus scan playheads and the spray region. |
| **Exploration** | Randomise and presets turn tweaking into discovery. | Dice-style `Randomize` (constrained to musical ranges), 17 categorised factory presets, user presets, `Keep Sample` lock. |
| **Sample workflow** | Bring your own sounds instantly. | Drag & drop WAV/AIFF/FLAC from Live's browser or Finder, Load menu, 3 built-in synthesised sources so it sounds good out of the box. |
