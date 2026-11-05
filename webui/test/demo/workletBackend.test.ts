import assert from 'node:assert/strict';
import { test } from 'node:test';
import { DEMO_BPM, STORAGE_KEY, workletBackend, type EnginePort, type PresetStorage } from '../../src/demo/workletBackend.ts';
import { info } from '../engine.ts';

const fakePort = () => {
  const sent: unknown[] = [];
  const port: EnginePort & { sent: unknown[] } = { sent, onmessage: null, postMessage: (message: unknown) => void sent.push(message) };
  return port;
};

const memoryStorage = (): PresetStorage & { items: Map<string, string> } => {
  const items = new Map<string, string>();
  return { items, getItem: (key) => items.get(key) ?? null, setItem: (key, value) => void items.set(key, value) };
};

const index = (id: string) => info.params.findIndex((p) => p.id === id);

test('MIDI and parameter changes made before the audio starts are sent in order once it does', () => {
  const backend = workletBackend(info, memoryStorage());
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

test('peaks from the worklet reach the meter', () => {
  const backend = workletBackend(info, memoryStorage());
  const port = fakePort();
  const peaks: number[] = [];
  backend.onPeak((peak) => peaks.push(peak));
  backend.attach(port);

  port.onmessage?.({ data: { type: 'peak', peak: 0.5 } } as MessageEvent);

  assert.deepEqual(peaks, [0.5]);
});

test('presets save, list in order, load and delete, under the demo storage key', async () => {
  const storage = memoryStorage();
  const backend = workletBackend(info, storage);

  backend.setValue('cutoff', 500);
  assert.equal(await backend.presets.save('Wide'), true);
  backend.setValue('cutoff', 900);
  assert.equal(await backend.presets.save('Dark'), true);
  assert.ok(storage.items.has(STORAGE_KEY));
  assert.deepEqual(await backend.presets.list(), ['Dark', 'Wide']);

  const heard: number[] = [];
  backend.subscribe('cutoff', (value) => heard.push(value));
  assert.equal(await backend.presets.load('Wide'), true);
  assert.equal(backend.getValue('cutoff'), 500);
  assert.deepEqual(heard, [500]);

  assert.equal(await backend.presets.remove('Wide'), true);
  assert.deepEqual(await backend.presets.list(), ['Dark']);
  assert.equal(await backend.presets.load('Wide'), false);
  assert.equal(await backend.presets.remove('Wide'), false);
});

test('a preset leaves parameters it lacks as they are, as PresetManager does', async () => {
  const storage = memoryStorage();
  storage.setItem(STORAGE_KEY, JSON.stringify({ Old: { cutoff: 300, gone: 1 } }));
  const backend = workletBackend(info, storage);
  backend.setValue('drive', 0.9);

  assert.equal(await backend.presets.load('Old'), true);
  assert.equal(backend.getValue('cutoff'), 300);
  assert.equal(backend.getValue('drive'), 0.9);
});

test('without storage, saving fails rather than throwing', async () => {
  const backend = workletBackend(info, undefined);

  assert.equal(await backend.presets.save('Nowhere'), false);
  assert.deepEqual(await backend.presets.list(), []);
});
