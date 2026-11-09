# SampleRealm: Reese

A mono Reese bass synth: two detuned oscillators and a sub, through drive and a lowpass filter, shaped
by an envelope. Drag a knob up or down to turn it. While you do, its name shows the value.

## Playing

One note sounds at a time. Playing a note while another is held slides to it, without restarting the
envelope. Letting go returns to the note still held.

## Oscillators

- OSC A and OSC B pick each oscillator's wave: square, saw or triangle.
- DETUNE pulls the two oscillators out of tune with each other. That beating is the Reese sound.
- UNISON stacks up to 8 voices per oscillator, spread by the same detune.
- SUB adds a sine an octave below.
- GLIDE sets how long a slide between held notes takes, up to 2 seconds. At 0 there's no slide.

## Tone

- DRIVE saturates the oscillators and sub before the filter.
- CUTOFF and RESONANCE set the lowpass filter. The 12dB button switches it to 24dB for a steeper slope.
- LFO RATE and LFO DEPTH sweep the cutoff. SYNC locks the LFO to the host's tempo, and the knob then
  picks a note length, from 1/16 to 4 bars.
- OUTPUT sets the overall level.

## Envelope

ATTACK, DECAY, SUSTAIN and RELEASE shape each note's level. The graph shows the shape.

## Meter

The meter at the top right shows the output peak. The light above it turns on when the output goes
over 0 dB and stays on for a moment.

## Presets

- The arrows step through the presets, and the name opens the full list.
- A * after the name means the sound has changed since the preset was loaded.
- Save overwrites the current user preset. For a factory preset it asks for a name.
- The ... menu has Init Preset, Save As, Rename and Delete. In the plugin it also has Show Preset Folder.
- Factory presets can't be renamed, deleted or overwritten.
