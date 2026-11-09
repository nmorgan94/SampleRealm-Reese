// This plugin's presets in the demo: its state, and factory files like the plugin's
import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { test } from 'node:test';
import { parsePreset } from '../../src/demo/localPresets.ts';
import { parseState, STORAGE_KEY } from '../../src/demo/presetState.ts';
import { workletBackend } from '../../src/demo/workletBackend.ts';
import { info } from '../engine.ts';

// Stand-ins, as in Tests/PresetTests.cpp
const preset = (name: string, category: string, cutoff: number) =>
  `<?xml version="1.0" encoding="UTF-8"?>
<PRESET name="${name}" category="${category}" author="SampleRealm" formatVersion="1">
  <Parameters>
    <PARAM id="cutoff" value="${cutoff}"/>
  </Parameters>
</PRESET>`;

const factory = [preset('Dark 2', 'Dark', 600), preset('Bright 1', 'Bright', 8000), preset('Dark 10', 'Dark', 200), preset('Dark 1', 'Dark', 300)];

// The same order as Tests/PresetTests.cpp
const FACTORY_NAMES = ['Bright 1', 'Dark 1', 'Dark 2', 'Dark 10'];

const storageWith = (stored: unknown) => {
  const items = new Map([[STORAGE_KEY, JSON.stringify(stored)]]);
  return { getItem: (key: string) => items.get(key) ?? null, setItem: (key: string, value: string) => void items.set(key, value) };
};

test("factory presets load into the engine in the plugin's order", async () => {
  const { presets, getValue, param, setValue } = workletBackend(info, factory);
  const list = await presets.list();
  assert.deepEqual(
    list.map((preset) => preset.name),
    FACTORY_NAMES
  );

  setValue('drive', 0.9);
  await presets.load(list.find((preset) => preset.name === 'Dark 1')!);
  assert.equal(getValue('cutoff'), 300);
  // Missing from the preset, so back at its default, as in the APVTS
  assert.equal(getValue('drive'), param('drive').defaultValue);

  await presets.loadInit();
  assert.equal(getValue('cutoff'), param('cutoff').defaultValue);
  assert.equal(presets.status().current.name, 'Init');
});

test("a stored preset loads past ids this engine doesn't have", async () => {
  const { presets, getValue } = workletBackend(info, factory, storageWith([{ name: 'Other', category: '', values: { gone: 1, cutoff: 900 } }]));

  await presets.load({ name: 'Other', category: '', isFactory: false });

  assert.equal(getValue('cutoff'), 900);
  assert.equal(presets.status().current.name, 'Other');
});

const presetFolder = resolve(import.meta.dirname, '../../../Assets/Presets');
const pluginFactory = readdirSync(presetFolder)
  .filter((file) => file.endsWith('.srreese'))
  .map((file) => readFileSync(resolve(presetFolder, file), 'utf8'));

test("the plugin's factory presets load every parameter, in range", async () => {
  const { presets, getValue, param } = workletBackend(info, pluginFactory);

  // The same order as Tests/PresetTests.cpp
  assert.deepEqual(
    (await presets.list()).map((preset) => preset.name),
    ['Classic Reese', 'Glide Reese', 'Wide Reese', 'Deep Rumble', 'Night Drive', 'Sub Weight', 'Grind', 'Tearout', 'Eighth Wobble', 'Slow Sweep', 'Triplet Wobble']
  );

  for (const xml of pluginFactory) {
    const { name, category, values } = parsePreset(xml, parseState);
    await presets.load({ name, category, isFactory: true });
    assert.deepEqual(Object.keys(values).sort(), info.params.map((param) => param.id).sort(), name);

    for (const [id, value] of Object.entries(values)) {
      assert.ok(value >= param(id).min && value <= param(id).max, `${name}: ${id} ${value}`);
      assert.equal(getValue(id), value, `${name}: ${id}`);
    }
  }
});
