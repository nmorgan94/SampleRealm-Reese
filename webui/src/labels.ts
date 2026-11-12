// Mirrors the text configureKnob gives the slider in Source/PluginEditor.cpp
import { formatValue } from '@samplerealm/modules/editor/params.ts';
import type { ParamInfo } from './backend.ts';

export const isWave = (id: string) => id === 'oscAWave' || id === 'oscBWave';

export const dragText = (param: ParamInfo, value: number) => {
  if (param.kind === 'choice') return formatValue(param, value).toUpperCase();
  return formatValue(param, value);
};
