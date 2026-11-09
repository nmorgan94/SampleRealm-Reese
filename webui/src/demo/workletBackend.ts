import { listenerSet, paramLookup, type Level, type PluginBackend, type UiInfo } from '../backend.ts';
import { localPresets, type PresetStorage } from './localPresets.ts';
import { INIT_STATE, parseState, STORAGE_KEY, type PresetState } from './presetState.ts';

// WASI imports are never called
export const stubImports = () => {
  const stub = new Proxy({}, { get: () => () => 0 });
  return new Proxy({}, { get: () => stub }) as WebAssembly.Imports;
};

export type Engine = Record<string, CallableFunction> & { memory: WebAssembly.Memory };

export const readUiInfo = (engine: Engine) => {
  const bytes = new Uint8Array(engine.memory.buffer, engine.uiInfo());
  return JSON.parse(new TextDecoder().decode(bytes.subarray(0, bytes.indexOf(0)))) as UiInfo;
};

export async function loadEngine(url: string): Promise<{ wasm: ArrayBuffer; info: UiInfo }> {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`${url}: ${response.status}`);

  const wasm = await response.arrayBuffer();
  const { instance } = await WebAssembly.instantiate(wasm, stubImports());
  const engine = instance.exports as Engine;
  engine._initialize?.();
  return { wasm, info: readUiInfo(engine) };
}

// The tempo the demo plays the host at
export const DEMO_BPM = 174;

const NOTE_ON = 0x90;
const NOTE_OFF = 0x80;
const VELOCITY = 100;

// The node's port, so tests can stand in for it
export type EnginePort = Pick<MessagePort, 'postMessage'> & { onmessage: ((e: MessageEvent) => void) | null };

export type WorkletBackend = PluginBackend & {
  noteOn(note: number): void;
  noteOff(note: number): void;
  midi(bytes: ArrayLike<number>): void;
  attach(port: EnginePort): void;
};

export function workletBackend(info: UiInfo, factoryPresets: string[] = [], storage?: PresetStorage): WorkletBackend {
  const entry = paramLookup(new Map(info.params.map((param, index) => [param.id, { param, index, value: param.defaultValue, listeners: listenerSet<[number]>() }])));
  const levelListeners = listenerSet<[Level]>();
  let port: EnginePort | null = null;
  // Sent once the worklet attaches
  let pending: unknown[] = [];

  const post = (message: unknown) => {
    if (port) port.postMessage(message);
    else pending.push(message);
  };

  const apply = (id: string, value: number) => {
    const found = entry(id);
    post({ type: 'param', index: found.index, value });
    found.value = value;
    found.listeners.notify(value);
  };

  const presets = localPresets<PresetState>({
    storageKey: STORAGE_KEY,
    storage,
    factory: factoryPresets,
    parse: parseState,
    init: INIT_STATE,
    capture: () => ({ values: Object.fromEntries(info.params.map(({ id }) => [id, entry(id).value])) }),
    // Ids this engine doesn't have are skipped, as the APVTS skips them
    apply: ({ values }) => info.params.forEach(({ id, defaultValue }) => apply(id, values[id] ?? defaultValue)),
  });

  const midi = (bytes: ArrayLike<number>) => {
    const [status = 0, data1 = 0, data2 = 0] = Array.from(bytes);
    post({ type: 'midi', bytes: [status, data1, data2] });
  };

  post({ type: 'transport', bpm: DEMO_BPM, playing: false, ppq: 0 });

  return {
    info,
    running: 'Web demo',
    presets,
    param: (id) => entry(id).param,
    getValue: (id) => entry(id).value,
    setValue: (id, value) => {
      apply(id, value);
      presets.markModified();
    },
    beginGesture: () => {},
    endGesture: () => {},
    subscribe: (id, listener) => entry(id).listeners.add(listener),
    onLevel: levelListeners.add,
    onBpm: (listener) => {
      listener(DEMO_BPM);
      return () => {};
    },
    noteOn: (note) => midi([NOTE_ON, note, VELOCITY]),
    noteOff: (note) => midi([NOTE_OFF, note, 0]),
    midi,
    attach: (enginePort) => {
      port = enginePort;
      port.onmessage = ({ data }) => {
        if (data.type === 'level') levelListeners.notify({ peaks: data.peaks });
      };
      pending.forEach((message) => enginePort.postMessage(message));
      pending = [];
    },
  };
}
