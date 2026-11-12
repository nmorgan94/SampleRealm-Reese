import assert from 'node:assert/strict';
import { test } from 'node:test';
import { parseValue } from '@samplerealm/modules/editor/params.ts';
import { dragText } from '../src/labels.ts';
import { info, param } from './engine.ts';

test('while dragging, labels show the value as the native editor does', () => {
  assert.equal(dragText(param('oscAWave'), 0), 'SQUARE');
  assert.equal(dragText(param('oscBWave'), 2), 'TRI');
  assert.equal(dragText(param('lfoSyncRate'), 12), '1 BAR');
  assert.equal(dragText(param('unisonVoices'), 5), '5');
});

test('a value shown on a choice knob reads back as itself when its name is confirmed', () => {
  for (const choice of info.params.filter(({ kind }) => kind === 'choice')) {
    for (let index = 0; index <= choice.max; index++) assert.equal(parseValue(choice, dragText(choice, index)), index, `${choice.id} ${index}`);
  }
});
