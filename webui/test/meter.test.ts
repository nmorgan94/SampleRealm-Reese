import assert from 'node:assert/strict';
import { test } from 'node:test';
import { meterBallistics, TICK_MS } from '../src/meter.ts';

test('the level jumps up and falls by 5% a tick', () => {
  const tick = meterBallistics();

  assert.equal(tick(0.5).level, 0.5);
  assert.equal(tick(0).level, 0.475);
  assert.equal(tick(0.9).level, 0.9);
});

test('a clip holds for 500 ms after the last loud tick', () => {
  const tick = meterBallistics();

  assert.equal(tick(1).clipping, true);
  const ticksHeld = Array.from({ length: 20 }, () => tick(0).clipping).filter(Boolean).length;
  assert.equal(ticksHeld, Math.ceil(500 / TICK_MS) - 1);
});
