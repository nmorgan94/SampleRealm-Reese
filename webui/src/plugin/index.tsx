import { createRoot } from 'react-dom/client';
import { juceBackend } from './juceBackend.ts';
import { ReeseEditor } from '../ReeseEditor.tsx';
import './styles.css';

const root = createRoot(document.getElementById('root')!);

try {
  root.render(<ReeseEditor backend={juceBackend()} />);
} catch (error) {
  console.error(error);
  root.render(<pre className="p-5 text-fg">{`The editor couldn't connect to the plugin.\n${error}`}</pre>);
}
