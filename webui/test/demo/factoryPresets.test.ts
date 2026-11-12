// This plugin's factory preset files in the demo
import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { test } from 'node:test';
import { parsePreset } from '@samplerealm/modules/demo/localPresets.ts';
import { parseState, workletBackend } from '@samplerealm/modules/demo/workletBackend.ts';
import { info } from '../engine.ts';

const presetFolder = resolve(import.meta.dirname, '../../../Assets/Presets');
const factoryPresets = readdirSync(presetFolder)
  .filter((file) => file.endsWith('.srreese'))
  .map((file) => readFileSync(resolve(presetFolder, file), 'utf8'));

// The same order as Tests/PresetTests.cpp
const FACTORY_NAMES = ['Classic Reese', 'Glide Reese', 'Wide Reese', 'Deep Rumble', 'Night Drive', 'Sub Weight', 'Grind', 'Tearout', 'Eighth Wobble', 'Slow Sweep', 'Triplet Wobble'];

test("the plugin's factory presets load every parameter, in range, in the plugin's order", async () => {
  const { presets, getValue, param } = workletBackend(info, { storageKey: 'test-presets', factoryPresets });

  assert.deepEqual((await presets.list()).map((preset) => preset.name), FACTORY_NAMES);

  for (const xml of factoryPresets) {
    const { name, category, values } = parsePreset(xml, parseState);
    await presets.load({ name, category, isFactory: true });
    assert.deepEqual(Object.keys(values).sort(), info.params.map(({ id }) => id).sort(), name);

    for (const [id, value] of Object.entries(values)) {
      assert.ok(value >= param(id).min && value <= param(id).max, `${name}: ${id} ${value}`);
      assert.equal(getValue(id), value, `${name}: ${id}`);
    }
  }
});
