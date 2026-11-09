import assert from 'node:assert/strict';
import { test } from 'node:test';
import { DEMO_BPM, workletBackend, type EnginePort } from '../../src/demo/workletBackend.ts';
import { info } from '../engine.ts';

const fakePort = () => {
  const sent: unknown[] = [];
  const port: EnginePort & { sent: unknown[] } = { sent, onmessage: null, postMessage: (message: unknown) => void sent.push(message) };
  return port;
};

const index = (id: string) => info.params.findIndex((p) => p.id === id);

test('MIDI and parameter changes made before the audio starts are sent in order once it does', () => {
  const backend = workletBackend(info);
  backend.noteOn(48);
  backend.setValue('cutoff', 500);
  backend.noteOff(48);

  const port = fakePort();
  backend.attach(port);
  backend.midi([0xe0, 0, 0x40]);

  assert.deepEqual(port.sent, [
    { type: 'transport', bpm: DEMO_BPM, playing: false, ppq: 0 },
    { type: 'midi', bytes: [0x90, 48, 100] },
    { type: 'param', index: index('cutoff'), value: 500 },
    { type: 'midi', bytes: [0x80, 48, 0] },
    { type: 'midi', bytes: [0xe0, 0, 0x40] },
  ]);
});

test('levels from the worklet reach the meter', () => {
  const backend = workletBackend(info);
  const port = fakePort();
  const peaks: number[][] = [];
  backend.onLevel((level) => peaks.push(level.peaks));
  backend.attach(port);

  port.onmessage?.({ data: { type: 'level', peaks: [0.5] } } as MessageEvent);

  assert.deepEqual(peaks, [[0.5]]);
});

test('moving a parameter marks the preset modified', () => {
  const backend = workletBackend(info);
  assert.equal(backend.presets.status().modified, false);

  backend.setValue('cutoff', 500);

  assert.equal(backend.presets.status().modified, true);
});
