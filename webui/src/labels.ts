// Mirrors AudioPluginAudioProcessorEditor::updateSliderLabel
import type { ParamInfo } from './backend.ts';
import { formatValue } from './params.ts';

const WAVE_NAMES = ['SQUARE', 'SAW', 'TRI'];

export const isWave = (id: string) => id === 'oscAWave' || id === 'oscBWave';

export const dragText = (param: ParamInfo, value: number) => {
  if (isWave(param.id)) return WAVE_NAMES[Math.round(value)] ?? '';
  if (param.kind === 'choice') return formatValue(param, value).toUpperCase();
  return formatValue(param, value);
};
