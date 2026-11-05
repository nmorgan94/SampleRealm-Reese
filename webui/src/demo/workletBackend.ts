import { paramLookup, type PluginBackend, type UiInfo } from '../backend.ts';

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

export const STORAGE_KEY = 'samplerealm-reese-presets';

// The tempo the demo plays the host at
export const DEMO_BPM = 174;

const NOTE_ON = 0x90;
const NOTE_OFF = 0x80;
const VELOCITY = 100;

export type PresetStorage = Pick<Storage, 'getItem' | 'setItem'>;

type StoredPresets = Record<string, Record<string, number>>;

// Storage can throw, e.g. in a private window
const browserStorage = (): PresetStorage | undefined => {
  try {
    return globalThis.localStorage;
  } catch {
    return undefined;
  }
};

// The node's port, so tests can stand in for it
export type EnginePort = Pick<MessagePort, 'postMessage'> & { onmessage: ((e: MessageEvent) => void) | null };

export type WorkletBackend = PluginBackend & {
  noteOn(note: number): void;
  noteOff(note: number): void;
  midi(bytes: ArrayLike<number>): void;
  attach(port: EnginePort): void;
};

export function workletBackend(info: UiInfo, storage: PresetStorage | undefined = browserStorage()): WorkletBackend {
  const entry = paramLookup(new Map(info.params.map((param, index) => [param.id, { param, index, value: param.defaultValue }])));
  const listeners = new Map<string, Set<(value: number) => void>>();
  const peakListeners = new Set<(peak: number) => void>();
  let port: EnginePort | null = null;
  // Sent once the worklet attaches
  let pending: unknown[] = [];

  const post = (message: unknown) => {
    if (port) port.postMessage(message);
    else pending.push(message);
  };

  const setValue = (id: string, value: number) => {
    const found = entry(id);
    post({ type: 'param', index: found.index, value });
    found.value = value;
    listeners.get(id)?.forEach((listener) => listener(value));
  };

  const readPresets = (): StoredPresets => {
    try {
      return JSON.parse(storage?.getItem(STORAGE_KEY) ?? '{}') as StoredPresets;
    } catch {
      return {};
    }
  };

  const writePresets = (presets: StoredPresets) => {
    try {
      storage?.setItem(STORAGE_KEY, JSON.stringify(presets));
      return storage !== undefined;
    } catch {
      return false;
    }
  };

  const midi = (bytes: ArrayLike<number>) => {
    const [status = 0, data1 = 0, data2 = 0] = Array.from(bytes);
    post({ type: 'midi', bytes: [status, data1, data2] });
  };

  post({ type: 'transport', bpm: DEMO_BPM, playing: false, ppq: 0 });

  return {
    info,
    param: (id) => entry(id).param,
    getValue: (id) => entry(id).value,
    setValue,
    beginGesture: () => {},
    endGesture: () => {},
    subscribe: (id, listener) => {
      const set = listeners.get(id) ?? new Set();
      listeners.set(id, set.add(listener));
      return () => set.delete(listener);
    },
    onPeak: (listener) => {
      peakListeners.add(listener);
      return () => peakListeners.delete(listener);
    },
    onBpm: (listener) => {
      listener(DEMO_BPM);
      return () => {};
    },
    presets: {
      list: async () => Object.keys(readPresets()).sort((a, b) => a.localeCompare(b)),
      // As PresetManager::loadPreset: parameters the preset lacks keep their values
      load: async (name) => {
        const stored = readPresets()[name];
        if (!stored) return false;
        info.params.forEach(({ id }) => id in stored && setValue(id, stored[id]!));
        return true;
      },
      save: async (name) => {
        if (!name) return false;
        return writePresets({ ...readPresets(), [name]: Object.fromEntries(info.params.map(({ id }) => [id, entry(id).value])) });
      },
      remove: async (name) => {
        const presets = readPresets();
        if (!(name in presets)) return false;
        delete presets[name];
        return writePresets(presets);
      },
    },
    noteOn: (note) => midi([NOTE_ON, note, VELOCITY]),
    noteOff: (note) => midi([NOTE_OFF, note, 0]),
    midi,
    attach: (enginePort) => {
      port = enginePort;
      port.onmessage = ({ data }) => {
        if (data.type === 'peak') peakListeners.forEach((listener) => listener(data.peak));
      };
      pending.forEach((message) => enginePort.postMessage(message));
      pending = [];
    },
  };
}
