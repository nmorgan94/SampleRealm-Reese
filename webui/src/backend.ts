type ParamKind = 'real' | 'integer' | 'choice';

export interface ParamInfo {
  id: string;
  name: string;
  kind: ParamKind;
  min: number;
  max: number;
  defaultValue: number;
  step: number;
  skew: number;
  unit: string;
  choices: string[];
}

export interface UiInfo {
  version: string;
  commit: string;
  build: 'Debug' | 'Release';
  params: ParamInfo[];
}

// Mirrors Source/PresetManager.h. No preset is current until one is loaded or saved
export interface Presets {
  list(): Promise<string[]>;
  load(name: string): Promise<boolean>;
  save(name: string): Promise<boolean>;
  remove(name: string): Promise<boolean>;
}

export interface PluginBackend {
  info: UiInfo;
  presets: Presets;
  param(id: string): ParamInfo;
  // Real values, not 0..1
  getValue(id: string): number;
  setValue(id: string, value: number): void;
  beginGesture(id: string): void;
  endGesture(id: string): void;
  subscribe(id: string, listener: (value: number) => void): () => void;
  // Linear peak since the last one
  onPeak(listener: (peak: number) => void): () => void;
  // The tempo the LFO syncs to, 0 until known
  onBpm(listener: (bpm: number) => void): () => void;
}

export const paramLookup = <T>(entries: Map<string, T>) => (id: string) => {
  const found = entries.get(id);
  if (!found) throw new Error(`No parameter ${id}`);
  return found;
};
