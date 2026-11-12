import { createRoot } from 'react-dom/client';
import { onEvent, pluginBackend } from '@samplerealm/modules/plugin/pluginBackend.ts';
import type { ReeseBackend } from '../backend.ts';
import { ReeseEditor } from '../ReeseEditor.tsx';
import './styles.css';

const root = createRoot(document.getElementById('root')!);

try {
  // The plugin's onTick emits it
  const backend: ReeseBackend = { ...pluginBackend(), onBpm: (listener) => onEvent<number>('bpm', listener) };
  root.render(<ReeseEditor backend={backend} />);
} catch (error) {
  console.error(error);
  root.render(<pre className="p-5 text-fg">{`The editor couldn't connect to the plugin.\n${error}`}</pre>);
}
