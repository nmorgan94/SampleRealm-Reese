import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { test } from 'node:test';
import { localEnginePath } from '../webui/localEngine.ts';
import { readUiInfo, stubImports } from '../webui/src/demo/workletBackend.ts';

const sampleRate = 48000;
const blockSize = 128;
const noteOn = 0x90;
const noteOff = 0x80;
const pitchWheel = 0xe0;
const velocity = 100;

const wasm = new WebAssembly.Module(readFileSync(localEnginePath()));

function engine() {
  const { exports } = new WebAssembly.Instance(wasm, stubImports());
  exports._initialize?.();
  exports.prepare(sampleRate);

  const ids = readUiInfo(exports).params.map((p) => p.id);
  const set = (id, value) => {
    assert.ok(ids.includes(id), `no parameter ${id}`);
    exports.setParam(ids.indexOf(id), value);
  };

  const play = (seconds) => {
    const out = [];
    for (let n = 0; n < Math.round((seconds * sampleRate) / blockSize); n++) {
      out.push(...new Float32Array(exports.memory.buffer, exports.render(blockSize), blockSize));
    }
    return out;
  };

  return {
    ...exports,
    set,
    play,
    noteOn: (note) => exports.midi(noteOn, note, velocity),
    noteOff: (note) => exports.midi(noteOff, note, 0),
  };
}

// No detune, sub or drive: one clean waveform, without beating
function plain() {
  const synth = engine();
  synth.set('detune', 0);
  synth.set('sub', 0);
  synth.set('drive', 0);
  return synth;
}

// Triangles through a wide open filter, so zero crossings give the pitch
function pureTone() {
  const synth = plain();
  synth.set('oscAWave', 2);
  synth.set('oscBWave', 2);
  synth.set('lfoDepth', 0);
  synth.set('cutoff', 18000);
  return synth;
}

const peak = (samples) => Math.max(...samples.map(Math.abs));

const rms = (samples) => Math.sqrt(samples.reduce((sum, s) => sum + s * s, 0) / samples.length);

// From the first to the last rising zero crossing, each placed between its two samples
const frequency = (samples) => {
  const crossings = [];
  for (let i = 1; i < samples.length; i++) {
    if (samples[i - 1] < 0 && samples[i] >= 0) crossings.push(i - 1 + samples[i - 1] / (samples[i - 1] - samples[i]));
  }
  return ((crossings.length - 1) * sampleRate) / (crossings.at(-1) - crossings[0]);
};

const semitones = (ratio) => 12 * Math.log2(ratio);

// Cycles of the level's movement, from the RMS of 40 ms windows
const modulationCycles = (samples) => {
  const window = sampleRate / 25;
  const levels = [];
  for (let i = 0; i + window <= samples.length; i += window) levels.push(rms(samples.slice(i, i + window)));
  const mean = levels.reduce((a, b) => a + b, 0) / levels.length;
  let rising = 0;
  for (let i = 1; i < levels.length; i++) if (levels[i - 1] < mean && levels[i] >= mean) rising++;
  return rising;
};

test('the ui info carries the version, the commit, the build type and every parameter as ParameterSpecs defines it', () => {
  const info = readUiInfo(engine());

  assert.match(info.version, /^\d+\.\d+\.\d+$/);
  assert.match(info.commit, /^([0-9a-f]{7,}(-local)?|unknown)$/);
  assert.match(info.build, /^(Debug|Release)$/);
  assert.deepEqual(info.params.map((p) => p.id), [
    'output', 'detune', 'sub', 'cutoff', 'resonance', 'drive', 'lfoRate', 'lfoDepth',
    'lfoSync', 'filter24db', 'lfoSyncRate', 'glideTime', 'unisonVoices', 'oscAWave', 'oscBWave',
    'attack', 'decay', 'sustain', 'release',
  ]);
  assert.deepEqual(info.params.find((p) => p.id === 'lfoSyncRate'), {
    id: 'lfoSyncRate', name: 'LFO Sync Rate', kind: 'choice', min: 0, max: 14, defaultValue: 6,
    step: 1, skew: 1, unit: '', choices: ['1/16', '1/16T', '1/16D', '1/8', '1/8T', '1/8D', '1/4', '1/4T', '1/4D',
      '1/2', '1/2T', '1/2D', '1 Bar', '2 Bars', '4 Bars'],
  });
});

test('a note sounds, and dies away after its release', () => {
  const synth = engine();
  synth.set('release', 0.05);

  synth.noteOn(48);
  assert.ok(peak(synth.play(0.2)) > 0.05);
  assert.ok(synth.peak() > 0.05);

  synth.noteOff(48);
  synth.play(0.2);
  assert.equal(peak(synth.play(0.05)), 0);
});

test('notes play two octaves down', () => {
  const synth = pureTone();

  synth.noteOn(69);
  synth.play(0.1);

  assert.ok(Math.abs(semitones(frequency(synth.play(1)) / 110)) < 0.01);
});

test('the pitch wheel bends up to two semitones either way', () => {
  for (const [lsb, msb, expected] of [[0x7f, 0x7f, 2], [0, 0, -2]]) {
    const synth = pureTone();

    synth.noteOn(69);
    synth.midi(pitchWheel, lsb, msb);
    synth.play(0.1);

    assert.ok(Math.abs(semitones(frequency(synth.play(1)) / 110) - expected) < 0.01, `bend to ${expected}`);
  }
});

test('output scales the level', () => {
  const render = (output) => {
    const synth = engine();
    synth.set('output', output);
    synth.noteOn(48);
    return peak(synth.play(0.3));
  };

  assert.ok(Math.abs(render(0.5) / render(1) - 0.5) < 1e-6);
});

test('the 24 dB filter is darker than the 12 dB one', () => {
  const render = (filter24db) => {
    const synth = engine();
    synth.set('cutoff', 300);
    synth.set('lfoDepth', 0);
    synth.set('filter24db', filter24db);
    synth.noteOn(60);
    synth.play(0.1);
    return rms(synth.play(0.5));
  };

  assert.ok(render(1) < 0.8 * render(0));
});

test('a synced LFO follows the tempo', () => {
  const cycles = (bpm) => {
    const synth = plain();
    synth.set('cutoff', 400);
    synth.set('lfoDepth', 1);
    synth.set('lfoSync', 1);
    synth.set('lfoSyncRate', 6); // 1/4
    if (bpm) synth.setTransport(bpm, true, 0);
    synth.noteOn(72);
    synth.play(0.1);
    return modulationCycles(synth.play(2));
  };

  // A quarter note: once a second at 60 BPM
  for (const [bpm, expected] of [[60, 2], [120, 4], [240, 8]]) {
    const counted = cycles(bpm);
    assert.ok(Math.abs(counted - expected) <= 1, `${counted} cycles at ${bpm} BPM`);
  }

  // 174 BPM until the page sets one
  const counted = cycles();
  assert.ok(Math.abs(counted - 5.8) <= 1, `${counted} cycles at the default tempo`);
});

test('setParam holds values to the parameter ranges', () => {
  const render = (values) => {
    const synth = engine();
    synth.set('lfoSync', 1);
    for (const [id, value] of Object.entries(values)) synth.set(id, value);
    synth.noteOn(48);
    return synth.play(0.2);
  };

  assert.deepEqual(render({ unisonVoices: 12, lfoSyncRate: 20 }), render({ unisonVoices: 8, lfoSyncRate: 14 }));
  assert.deepEqual(render({ unisonVoices: -3, lfoSyncRate: -5 }), render({ unisonVoices: 1, lfoSyncRate: 0 }));
  assert.deepEqual(render({ cutoff: NaN, output: Infinity }), render({ output: 1 }));
});

test('bad input is ignored', () => {
  const synth = engine();

  synth.setParam(-1, 1);
  synth.setParam(1000, 1);
  synth.midi(0x40, 60, 100);
  synth.midi(0xf8, 0, 0);
  assert.equal(peak(synth.play(0.1)), 0);

  synth.noteOn(48);
  assert.doesNotThrow(() => synth.render(100000));
  assert.doesNotThrow(() => synth.render(-5));
  assert.ok(peak(synth.play(0.1)) > 0);
});

const hz = (note) => 440 * 2 ** ((note - 24 - 69) / 12);
const near = (actual, note) => Math.abs(semitones(actual / hz(note))) < 0.05;

test('a note played while another is held glides to it without restarting the envelope', () => {
  const synth = pureTone();
  synth.set('glideTime', 0.1);

  synth.noteOn(60);
  const before = synth.play(0.3);
  synth.noteOn(67);
  const after = synth.play(0.01);
  const gliding = synth.play(0.05);
  synth.play(1);
  const settled = synth.play(0.5);

  assert.ok(rms(after) > 0.8 * rms(before.slice(-480)), 'the level carries on');
  const midGlide = frequency(gliding);
  assert.ok(midGlide > hz(60) * 1.01 && midGlide < hz(67) * 0.99, `${midGlide} Hz mid glide`);
  assert.ok(near(frequency(settled), 67));
});

test('a note played with none held starts at its pitch, with no glide', () => {
  const synth = pureTone();
  synth.set('glideTime', 1);
  synth.set('release', 0.5);

  synth.noteOn(60);
  synth.play(0.2);
  synth.noteOff(60);
  synth.play(0.05);
  synth.noteOn(67);

  assert.ok(near(frequency(synth.play(0.1)), 67));
});

test('releasing the newest note returns to the one still held', () => {
  const synth = pureTone();

  synth.noteOn(60);
  synth.noteOn(67);
  synth.play(0.2);
  synth.noteOff(67);
  synth.play(0.05);

  const returned = synth.play(0.5);
  assert.ok(peak(returned) > 0.05, 'still sounding');
  assert.ok(near(frequency(returned), 60));
});

test('the sound releases only when the last held note goes', () => {
  const synth = pureTone();
  synth.set('release', 0.05);

  synth.noteOn(60);
  synth.noteOn(67);
  synth.play(0.2);
  synth.noteOff(60);
  synth.play(0.2);

  const stillHeld = synth.play(0.2);
  assert.ok(peak(stillHeld) > 0.05, 'still sounding');
  assert.ok(near(frequency(stillHeld), 67));

  synth.noteOff(67);
  synth.play(0.3);
  assert.equal(peak(synth.play(0.05)), 0);
});

test('All Notes Off releases every held note', () => {
  const synth = engine();
  synth.set('release', 0.05);

  synth.noteOn(60);
  synth.noteOn(67);
  synth.play(0.2);
  synth.midi(0xb0, 123, 0);
  synth.play(0.3);
  assert.equal(peak(synth.play(0.05)), 0);

  // Nothing is left held to come back
  synth.noteOn(48);
  synth.noteOff(48);
  synth.play(0.3);
  assert.equal(peak(synth.play(0.05)), 0);
});
