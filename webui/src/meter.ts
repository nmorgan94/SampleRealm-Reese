// Mirrors AudioPluginAudioProcessorEditor::timerCallback()
export const TICK_MS = 30;
const DECAY = 0.95;
const CLIP_HOLD_MS = 500;

export interface MeterState {
  level: number;
  clipping: boolean;
}

export function meterBallistics() {
  let level = 0;
  let clipping = false;
  let clipTimer = 0;

  // Takes the loudest sample since the last tick
  return (peak: number): MeterState => {
    level = Math.max(peak, level * DECAY);
    // Under a pixel, and lets the meter stop redrawing
    if (level < 1e-3) level = 0;

    if (peak >= 1) {
      clipping = true;
      clipTimer = CLIP_HOLD_MS;
    } else if (clipTimer > 0) {
      clipTimer -= TICK_MS;
      if (clipTimer <= 0) clipping = false;
    }

    return { level, clipping };
  };
}
