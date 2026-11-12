import assert from 'node:assert/strict';
import { test } from 'node:test';
import { formatValue, fromProportion } from '@samplerealm/modules/editor/params.ts';
import { info, param } from './engine.ts';

// From the Catch2 snapshot in Tests/ReeseTests.cpp
const pluginText: [id: string, proportion: number, text: string][] = [
  ['output', 0, '0.00'],
  ['output', 0.5, '0.50'],
  ['output', 1, '1.00'],
  ['detune', 0, '0.00'],
  ['detune', 0.5, '20.00'],
  ['detune', 1, '40.00'],
  ['sub', 0, '0.000'],
  ['sub', 0.5, '0.500'],
  ['sub', 1, '1.000'],
  ['cutoff', 0, '40.00'],
  ['cutoff', 0.5, '2518.68'],
  ['cutoff', 1, '18000.00'],
  ['resonance', 0, '0.100'],
  ['resonance', 0.5, '0.650'],
  ['resonance', 1, '1.200'],
  ['drive', 0, '0.000'],
  ['drive', 0.5, '0.500'],
  ['drive', 1, '1.000'],
  ['lfoRate', 0, '0.050'],
  ['lfoRate', 0.5, '2.803'],
  ['lfoRate', 1, '20.000'],
  ['lfoDepth', 0, '0.000'],
  ['lfoDepth', 0.5, '0.500'],
  ['lfoDepth', 1, '1.000'],
  ['lfoSync', 0, 'Off'],
  ['lfoSync', 0.5, 'Off'],
  ['lfoSync', 1, 'On'],
  ['filter24db', 0, 'Off'],
  ['filter24db', 0.5, 'Off'],
  ['filter24db', 1, 'On'],
  ['lfoSyncRate', 0, '1/16'],
  ['lfoSyncRate', 0.5, '1/4T'],
  ['lfoSyncRate', 1, '4 Bars'],
  ['glideTime', 0, '0.000'],
  ['glideTime', 0.5, '0.500'],
  ['glideTime', 1, '2.000'],
  ['unisonVoices', 0, '1'],
  ['unisonVoices', 0.5, '4'],
  ['unisonVoices', 1, '8'],
  ['oscAWave', 0, 'Square'],
  ['oscAWave', 0.5, 'Saw'],
  ['oscAWave', 1, 'Tri'],
  ['oscBWave', 0, 'Square'],
  ['oscBWave', 0.5, 'Saw'],
  ['oscBWave', 1, 'Tri'],
  ['attack', 0, '0.001'],
  ['attack', 0.5, '0.026'],
  ['attack', 1, '0.100'],
  ['decay', 0, '0.010'],
  ['decay', 0.5, '0.185'],
  ['decay', 1, '1.000'],
  ['sustain', 0, '0.00'],
  ['sustain', 0.5, '0.50'],
  ['sustain', 1, '1.00'],
  ['release', 0, '0.010'],
  ['release', 0.5, '0.362'],
  ['release', 1, '2.000'],
];

test('every parameter has a row in pluginText', () => {
  for (const { id } of info.params) assert.ok(pluginText.some(([row]) => row === id), id);
});

test('every value reads as the plugin shows it, at the same knob position', () => {
  for (const [id, proportion, text] of pluginText) {
    assert.equal(formatValue(param(id), fromProportion(param(id), proportion)), text, `${id} at ${proportion}`);
  }
});
