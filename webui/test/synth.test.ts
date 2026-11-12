// The engine's notes and sound. The plugin's Catch2 tests cover parameters, state and finite output
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { blockSize, rms, sampleRate } from '@samplerealm/modules/testing/engine.ts';
import { engine as monoEngine } from './engine.ts';

const NOTE_ON = 0x90;
const NOTE_OFF = 0x80;
const PITCH_WHEEL = 0xe0;
const VELOCITY = 100;
const UNPREPARED_RATE = 44100;

// Played a block at a time, as the demo's worklet plays it. Null: never prepared, so at the engine's default rate
function synth(rate: number | null = sampleRate) {
  const { engine, set, renderBlocks } = monoEngine(rate);
  const blocksOf = (seconds: number) => Math.round((seconds * (rate ?? UNPREPARED_RATE)) / blockSize);
  const play = (seconds: number) => Array.from(renderBlocks(blocksOf(seconds) * blockSize)[0]!);

  return {
    engine,
    set,
    block: () => Array.from(renderBlocks(blockSize)[0]!),
    play,
    noteOn: (note: number) => engine.midi(NOTE_ON, note, VELOCITY),
    noteOff: (note: number) => engine.midi(NOTE_OFF, note, 0),
  };
}

// No detune, sub or drive: one clean waveform, without beating
function plain() {
  const plainSynth = synth();
  plainSynth.set('detune', 0);
  plainSynth.set('sub', 0);
  plainSynth.set('drive', 0);
  return plainSynth;
}

// Triangles through a wide open filter, so zero crossings give the pitch
function pureTone() {
  const toneSynth = plain();
  toneSynth.set('oscAWave', 2);
  toneSynth.set('oscBWave', 2);
  toneSynth.set('lfoDepth', 0);
  toneSynth.set('cutoff', 18000);
  return toneSynth;
}

const peak = (samples: number[]) => Math.max(...samples.map(Math.abs));


// From the first to the last rising zero crossing, each placed between its two samples
const frequency = (samples: number[]) => {
  const crossings: number[] = [];
  for (let index = 1; index < samples.length; index++) {
    const before = samples[index - 1]!;
    const after = samples[index]!;
    if (before < 0 && after >= 0) crossings.push(index - 1 + before / (before - after));
  }
  return ((crossings.length - 1) * sampleRate) / (crossings.at(-1)! - crossings[0]!);
};

const semitones = (ratio: number) => 12 * Math.log2(ratio);

// Radix-2 FFT of a power-of-two length, as the level in each bin
const spectrum = (samples: number[]) => {
  const length = samples.length;
  const real = Float64Array.from(samples);
  const imaginary = new Float64Array(length);

  for (let index = 1, swapWith = 0; index < length; index++) {
    let bit = length >> 1;
    for (; swapWith & bit; bit >>= 1) swapWith ^= bit;
    swapWith ^= bit;
    if (index < swapWith) {
      [real[index], real[swapWith]] = [real[swapWith]!, real[index]!];
    }
  }

  for (let size = 2; size <= length; size *= 2) {
    for (let step = 0; step < size / 2; step++) {
      const cos = Math.cos((2 * Math.PI * step) / size);
      const sin = -Math.sin((2 * Math.PI * step) / size);
      for (let first = step; first < length; first += size) {
        const second = first + size / 2;
        const rotatedReal = real[second]! * cos - imaginary[second]! * sin;
        const rotatedImaginary = real[second]! * sin + imaginary[second]! * cos;
        real[second] = real[first]! - rotatedReal;
        imaginary[second] = imaginary[first]! - rotatedImaginary;
        real[first] = real[first]! + rotatedReal;
        imaginary[first] = imaginary[first]! + rotatedImaginary;
      }
    }
  }

  return real.map((value, bin) => Math.hypot(value, imaginary[bin]!));
};

// The loudest tone below the limit that is no harmonic of the note, in dB against its fundamental
const loudestAlias = (samples: number[], fundamental: number, limitHz: number) => {
  const binWidth = sampleRate / samples.length;
  const windowed = samples.map((sample, index) => sample * (0.5 - 0.5 * Math.cos((2 * Math.PI * index) / samples.length)));

  // The fundamental sits between bins, so it is measured at its own frequency
  let fundamentalReal = 0;
  let fundamentalImaginary = 0;
  windowed.forEach((sample, index) => {
    const angle = (2 * Math.PI * fundamental * index) / sampleRate;
    fundamentalReal += sample * Math.cos(angle);
    fundamentalImaginary += sample * Math.sin(angle);
  });

  const levels = spectrum(windowed);
  let loudest = 0;
  for (let bin = Math.ceil(40 / binWidth); bin * binWidth < limitHz; bin++) {
    const fromHarmonic = Math.abs(bin * binWidth - fundamental * Math.round((bin * binWidth) / fundamental));
    // Clear of each harmonic's own spread in the window
    if (fromHarmonic > 16 * binWidth) loudest = Math.max(loudest, levels[bin]!);
  }
  return 20 * Math.log10(loudest / Math.hypot(fundamentalReal, fundamentalImaginary));
};

// Cycles of the level's movement, from the RMS of 40 ms windows
const modulationCycles = (samples: number[]) => {
  const window = sampleRate / 25;
  const levels: number[] = [];
  for (let start = 0; start + window <= samples.length; start += window) levels.push(rms(samples.slice(start, start + window)));
  const mean = levels.reduce((sum, level) => sum + level, 0) / levels.length;
  let rising = 0;
  for (let index = 1; index < levels.length; index++) if (levels[index - 1]! < mean && levels[index]! >= mean) rising++;
  return rising;
};

test('the ui info carries the version, the commit, the build type and every parameter as ParameterSpecs defines it', () => {
  const { info: fresh } = monoEngine();

  assert.match(fresh.version, /^\d+\.\d+\.\d+$/);
  assert.match(fresh.commit, /^([0-9a-f]{7,}(-local)?|unknown)$/);
  assert.match(fresh.build, /^(Debug|Release)$/);
  assert.deepEqual(fresh.params.map((param) => param.id), [
    'output', 'detune', 'sub', 'cutoff', 'resonance', 'drive', 'lfoRate', 'lfoDepth',
    'lfoSync', 'filter24db', 'lfoSyncRate', 'glideTime', 'unisonVoices', 'oscAWave', 'oscBWave',
    'attack', 'decay', 'sustain', 'release',
  ]);
  assert.deepEqual(fresh.params.find((param) => param.id === 'lfoSyncRate'), {
    id: 'lfoSyncRate', name: 'LFO Sync Rate', kind: 'choice', min: 0, max: 14, defaultValue: 6,
    step: 1, skew: 1, unit: '', choices: ['1/16', '1/16T', '1/16D', '1/8', '1/8T', '1/8D', '1/4', '1/4T', '1/4D',
      '1/2', '1/2T', '1/2D', '1 Bar', '2 Bars', '4 Bars'],
  });
  assert.deepEqual(fresh.params.find((param) => param.id === 'oscAWave'), {
    id: 'oscAWave', name: 'Osc A Wave', kind: 'choice', min: 0, max: 2, defaultValue: 1,
    step: 1, skew: 1, unit: '', choices: ['Square', 'Saw', 'Tri'],
  });
});

test('a note sounds, and dies away after its release', () => {
  const played = synth();
  played.set('release', 0.05);

  played.noteOn(48);
  assert.ok(peak(played.play(0.2)) > 0.05);
  assert.ok(played.engine.peak(0) > 0.05);

  played.noteOff(48);
  played.play(0.2);
  assert.equal(peak(played.play(0.05)), 0);
});

test('notes play two octaves down', () => {
  const played = pureTone();

  played.noteOn(69);
  played.play(0.1);

  assert.ok(Math.abs(semitones(frequency(played.play(1)) / 110)) < 0.01);
});

test('the pitch wheel bends up to two semitones either way', () => {
  for (const [lsb, msb, expected] of [[0x7f, 0x7f, 2], [0, 0, -2]] as const) {
    const played = pureTone();

    played.noteOn(69);
    played.engine.midi(PITCH_WHEEL, lsb, msb);
    played.play(0.1);

    assert.ok(Math.abs(semitones(frequency(played.play(1)) / 110) - expected) < 0.01, `bend to ${expected}`);
  }
});

test('output scales the level', () => {
  const render = (output: number) => {
    const played = synth();
    played.set('output', output);
    played.noteOn(48);
    return peak(played.play(0.3));
  };

  assert.ok(Math.abs(render(0.5) / render(1) - 0.5) < 1e-6);
});

test('the 24 dB filter is darker than the 12 dB one', () => {
  const render = (filter24db: number) => {
    const played = synth();
    played.set('cutoff', 300);
    played.set('lfoDepth', 0);
    played.set('filter24db', filter24db);
    played.noteOn(60);
    played.play(0.1);
    return rms(played.play(0.5));
  };

  assert.ok(render(1) < 0.8 * render(0));
});

test('a synced LFO follows the tempo', () => {
  const cycles = (bpm?: number) => {
    const played = plain();
    played.set('cutoff', 400);
    played.set('lfoDepth', 1);
    played.set('lfoSync', 1);
    played.set('lfoSyncRate', 6); // 1/4
    if (bpm) played.engine.setTransport(bpm, true, 0);
    played.noteOn(72);
    played.play(0.1);
    return modulationCycles(played.play(2));
  };

  // A quarter note: once a second at 60 BPM
  for (const [bpm, expected] of [[60, 2], [120, 4], [240, 8]]) {
    const counted = cycles(bpm);
    assert.ok(Math.abs(counted - expected!) <= 1, `${counted} cycles at ${bpm} BPM`);
  }

  // 174 BPM until the page sets one
  const counted = cycles();
  assert.ok(Math.abs(counted - 5.8) <= 1, `${counted} cycles at the default tempo`);
});

test('setParam holds values to the parameter ranges', () => {
  const render = (values: Record<string, number>) => {
    const played = synth();
    played.set('lfoSync', 1);
    for (const [id, value] of Object.entries(values)) played.set(id, value);
    played.noteOn(48);
    return played.play(0.2);
  };

  assert.deepEqual(render({ unisonVoices: 12, lfoSyncRate: 20 }), render({ unisonVoices: 8, lfoSyncRate: 14 }));
  assert.deepEqual(render({ unisonVoices: -3, lfoSyncRate: -5 }), render({ unisonVoices: 1, lfoSyncRate: 0 }));
  assert.deepEqual(render({ cutoff: NaN, output: Infinity }), render({ output: 1 }));
});

test('bad input is ignored', () => {
  const played = synth();

  played.engine.setParam(-1, 1);
  played.engine.setParam(1000, 1);
  played.engine.midi(0x40, 60, 100);
  played.engine.midi(0xf8, 0, 0);
  assert.equal(peak(played.play(0.1)), 0);

  played.noteOn(48);
  assert.doesNotThrow(() => played.engine.render(100000));
  assert.doesNotThrow(() => played.engine.render(-5));
  assert.ok(peak(played.play(0.1)) > 0);
});

const hz = (note: number) => 440 * 2 ** ((note - 24 - 69) / 12);
const near = (actual: number, note: number) => Math.abs(semitones(actual / hz(note))) < 0.05;

test('the filter stays stable when the cutoff is above Nyquist', () => {
  for (const slope of [0, 1]) {
    const played = synth(22050);
    played.set('cutoff', 18000);
    played.set('lfoDepth', 1);
    played.set('resonance', 1.2);
    played.set('filter24db', slope);

    played.noteOn(48);
    const level = peak(played.play(0.5));
    assert.ok(level > 0.05 && level < 2, `peak ${level} with filter24db ${slope}`);
  }
});

test('rendering before prepare matches a 44.1 kHz engine', () => {
  const render = (played: ReturnType<typeof synth>) => {
    played.set('filter24db', 1);
    played.noteOn(48);
    return played.block();
  };

  assert.deepEqual(render(synth(null)), render(synth(UNPREPARED_RATE)));
});

test('saw and square stay clean high up the keyboard', () => {
  for (const [note, limitHz, ceiling] of [[96, 5000, -50], [127, 18000, -33]] as const) {
    for (const wave of [0, 1]) {
      const played = plain();
      played.set('oscAWave', wave);
      played.set('oscBWave', wave);
      played.set('lfoDepth', 0);
      played.set('cutoff', 18000);

      played.noteOn(note);
      played.play(0.2);
      const alias = loudestAlias(played.play(1).slice(0, 2 ** 15), hz(note), limitHz);
      assert.ok(alias < ceiling, `${alias.toFixed(1)} dB with wave ${wave} at note ${note}`);
    }
  }
});

test('a note played while another is held glides to it without restarting the envelope', () => {
  const played = pureTone();
  played.set('glideTime', 0.1);

  played.noteOn(60);
  const before = played.play(0.3);
  played.noteOn(67);
  const after = played.play(0.01);
  const gliding = played.play(0.05);
  played.play(1);
  const settled = played.play(0.5);

  assert.ok(rms(after) > 0.8 * rms(before.slice(-480)), 'the level carries on');
  const midGlide = frequency(gliding);
  assert.ok(midGlide > hz(60) * 1.01 && midGlide < hz(67) * 0.99, `${midGlide} Hz mid glide`);
  assert.ok(near(frequency(settled), 67));
});

test('a note played with none held starts at its pitch, with no glide', () => {
  const played = pureTone();
  played.set('glideTime', 1);
  played.set('release', 0.5);

  played.noteOn(60);
  played.play(0.2);
  played.noteOff(60);
  played.play(0.05);
  played.noteOn(67);

  assert.ok(near(frequency(played.play(0.1)), 67));
});

test('releasing the newest note returns to the one still held', () => {
  const played = pureTone();

  played.noteOn(60);
  played.noteOn(67);
  played.play(0.2);
  played.noteOff(67);
  played.play(0.05);

  const returned = played.play(0.5);
  assert.ok(peak(returned) > 0.05, 'still sounding');
  assert.ok(near(frequency(returned), 60));
});

test('the sound releases only when the last held note goes', () => {
  const played = pureTone();
  played.set('release', 0.05);

  played.noteOn(60);
  played.noteOn(67);
  played.play(0.2);
  played.noteOff(60);
  played.play(0.2);

  const stillHeld = played.play(0.2);
  assert.ok(peak(stillHeld) > 0.05, 'still sounding');
  assert.ok(near(frequency(stillHeld), 67));

  played.noteOff(67);
  played.play(0.3);
  assert.equal(peak(played.play(0.05)), 0);
});

test('All Notes Off releases every held note', () => {
  const played = synth();
  played.set('release', 0.05);

  played.noteOn(60);
  played.noteOn(67);
  played.play(0.2);
  played.engine.midi(0xb0, 123, 0);
  played.play(0.3);
  assert.equal(peak(played.play(0.05)), 0);

  // Nothing is left held to come back
  played.noteOn(48);
  played.noteOff(48);
  played.play(0.3);
  assert.equal(peak(played.play(0.05)), 0);
});

test('the one channel reads back for any channel the worklet asks for, and only the first has a level', () => {
  const played = synth();
  played.noteOn(48);
  played.play(0.1);

  assert.equal(played.engine.channel(1), played.engine.channel(0));
  assert.equal(played.engine.peak(1), 0);
  assert.ok(played.engine.peak(0) > 0);
});
