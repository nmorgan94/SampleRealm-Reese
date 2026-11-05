import assert from 'node:assert/strict';
import { test } from 'node:test';
import { dragText } from '../src/labels.ts';
import { param } from './engine.ts';

test('while dragging, labels show the value as the native editor does', () => {
  assert.equal(dragText(param('oscAWave'), 0), 'SQUARE');
  assert.equal(dragText(param('oscBWave'), 2), 'TRI');
  assert.equal(dragText(param('lfoSyncRate'), 12), '1 BAR');
  assert.equal(dragText(param('unisonVoices'), 5), '5');
});
