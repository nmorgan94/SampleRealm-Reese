import type { PluginBackend } from '@samplerealm/modules/backend.ts';

export * from '@samplerealm/modules/backend.ts';

// Plus the tempo the LFO syncs to, 0 until known: the plugin's from Plugin::onTick, the demo's its own
export type ReeseBackend = PluginBackend & { onBpm(listener: (bpm: number) => void): () => void };
