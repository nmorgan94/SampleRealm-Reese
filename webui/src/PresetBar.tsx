// Mirrors the preset controls in Source/PluginEditor.cpp and the dialogs in Source/PresetManager.h
import { useState } from 'react';
import type { PluginBackend } from './backend.ts';
import { TextButton } from './controls.tsx';
import { juceHeight } from './fonts.ts';
import { PRESET_BUTTON as button, PRESET_COMBO as combo, PRESET_GAP as gap, presetBar } from './layout.ts';

type Dialog =
  | { kind: 'save' }
  | { kind: 'delete'; name: string }
  | { kind: 'message'; title: string; text: string };

const DIALOG_BUTTON = 'h-6.5 min-w-16 px-2 text-xs';

export function PresetBar({ backend }: { backend: PluginBackend }) {
  const [current, setCurrent] = useState<string | null>(null);
  const [menu, setMenu] = useState<string[] | null>(null);
  const [dialog, setDialog] = useState<Dialog | null>(null);

  const load = async (name: string) => {
    setMenu(null);
    if (await backend.presets.load(name)) setCurrent(name);
  };

  const save = async (name: string) => {
    setDialog(null);
    if (!name) return;
    if (await backend.presets.save(name)) setCurrent(name);
    else setDialog({ kind: 'message', title: 'Error', text: 'Failed to save preset!' });
  };

  const remove = async (name: string) => {
    setDialog(null);
    if (await backend.presets.remove(name)) setCurrent(null);
    else setDialog({ kind: 'message', title: 'Error', text: 'Failed to delete preset!' });
  };

  return (
    <div className="absolute flex" style={{ left: presetBar.x, top: presetBar.y, height: presetBar.height, gap }}>
      <TextButton style={{ width: button, fontSize: 14 }} aria-label="Save preset" onClick={() => setDialog({ kind: 'save' })}>
        +
      </TextButton>

      <button
        type="button"
        className="relative flex cursor-pointer items-center rounded-sm border border-panel-border bg-linear-to-b from-panel to-panel-dark pl-1.5 text-left"
        style={{ width: combo, fontSize: juceHeight(14) }}
        aria-haspopup="listbox"
        onClick={async () => setMenu(await backend.presets.list())}
      >
        <span className={`flex-1 truncate ${current ? '' : 'opacity-50'}`}>{current ?? 'Select Preset'}</span>
        <svg className="mr-1.5 flex-none" width="8" height="5" viewBox="0 0 8 5" aria-hidden>
          <path d="M 0 0 L 4 5 L 8 0" fill="none" stroke="var(--color-accent)" strokeOpacity={0.9} strokeWidth="2" />
        </svg>
      </button>

      <TextButton
        style={{ width: button, fontSize: 14 }}
        aria-label="Delete preset"
        onClick={() =>
          setDialog(current ? { kind: 'delete', name: current } : { kind: 'message', title: 'No Preset Selected', text: 'Please select a preset to delete.' })
        }
      >
        -
      </TextButton>

      {menu && (
        <>
          <div className="fixed inset-0 z-20" onPointerDown={() => setMenu(null)} />
          <ul
            className="absolute z-21 max-h-60 overflow-y-auto border border-panel-border bg-linear-to-b from-panel to-panel-dark"
            style={{ left: button + gap, top: button + 2, width: combo, fontSize: juceHeight(14) }}
            role="listbox"
          >
            {menu.length === 0 && <li className="px-2 py-1 opacity-50">(no presets)</li>}
            {menu.map((name) => (
              <li
                key={name}
                className="cursor-pointer border border-transparent px-2 py-0.5 hover:border-accent/50 hover:bg-accent/20 hover:text-accent"
                role="option"
                aria-selected={name === current}
                onClick={() => load(name)}
              >
                {name}
              </li>
            ))}
          </ul>
        </>
      )}

      {dialog && (
        <div className="fixed inset-0 z-30 flex items-center justify-center bg-black/50">
          <form
            className="min-w-70 rounded-md border border-panel-border bg-panel p-4 font-sans"
            onSubmit={(e) => {
              e.preventDefault();
              if (dialog.kind === 'save') save(new FormData(e.currentTarget).get('presetName')?.toString() ?? '');
              else if (dialog.kind === 'delete') remove(dialog.name);
              else setDialog(null);
            }}
          >
            <h2 className="mb-2 text-sm font-bold">
              {dialog.kind === 'save' ? 'Save Preset' : dialog.kind === 'delete' ? 'Delete Preset' : dialog.title}
            </h2>
            {dialog.kind === 'save' && (
              <>
                <p className="mb-2 text-xs">Enter preset name:</p>
                <input
                  className="mb-3 w-full rounded-sm border border-panel-border bg-panel-dark px-1.5 py-1 text-xs text-fg outline-accent"
                  name="presetName"
                  aria-label="Preset Name"
                  autoFocus
                />
              </>
            )}
            {dialog.kind === 'delete' && <p className="mb-3 text-xs">{`Are you sure you want to delete '${dialog.name}'?`}</p>}
            {dialog.kind === 'message' && <p className="mb-3 text-xs">{dialog.text}</p>}
            <div className="flex justify-end gap-1.5">
              <TextButton type="submit" className={DIALOG_BUTTON} autoFocus={dialog.kind !== 'save'}>
                {dialog.kind === 'save' ? 'Save' : 'OK'}
              </TextButton>
              {dialog.kind !== 'message' && (
                <TextButton className={DIALOG_BUTTON} onClick={() => setDialog(null)}>
                  Cancel
                </TextButton>
              )}
            </div>
          </form>
        </div>
      )}
    </div>
  );
}
