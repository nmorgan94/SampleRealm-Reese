// Mirrors Source/ui/ADSRVisualizer.h
import { useId } from 'react';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';
import type { EditorBackend } from './backend.ts';

// The display's fixed sustain, in seconds
const SUSTAIN_TIME = 0.3;
const INSET = 10;

export function AdsrView({ backend, width, height }: { backend: EditorBackend; width: number; height: number }) {
  const id = useId();
  const attack = useParam(backend, 'attack').value;
  const decay = useParam(backend, 'decay').value;
  const sustain = useParam(backend, 'sustain').value;
  const release = useParam(backend, 'release').value;

  const left = INSET;
  const bottom = height - INSET;
  const top = INSET;
  const scale = (width - 2 * INSET) / (attack + decay + SUSTAIN_TIME + release);
  const attackX = left + attack * scale;
  const decayX = attackX + decay * scale;
  const sustainX = decayX + SUSTAIN_TIME * scale;
  const releaseX = sustainX + release * scale;
  const sustainY = bottom - (bottom - top) * sustain;

  const line = `M ${left} ${bottom} L ${attackX} ${top} L ${decayX} ${sustainY} L ${sustainX} ${sustainY} L ${releaseX} ${bottom}`;

  return (
    <svg className="pointer-events-none" width={width} height={height} aria-hidden>
      <defs>
        <linearGradient id={`${id}bg`} x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stopColor="var(--color-panel-dark)" stopOpacity={0.3} />
          <stop offset="1" stopColor="var(--color-panel)" stopOpacity={0.2} />
        </linearGradient>
        <linearGradient id={`${id}fill`} gradientUnits="userSpaceOnUse" x1="0" y1={top} x2="0" y2={bottom}>
          <stop offset="0" stopColor="var(--color-accent)" stopOpacity={0.15} />
          <stop offset="1" stopColor="var(--color-accent)" stopOpacity={0.05} />
        </linearGradient>
      </defs>
      <rect width={width} height={height} rx={8} fill={`url(#${id}bg)`} />
      <path d={`${line} Z`} fill={`url(#${id}fill)`} />
      <path d={line} stroke="var(--color-accent)" strokeOpacity={0.3} strokeWidth={3} fill="none" />
      <path d={line} stroke="var(--color-accent)" strokeOpacity={0.8} strokeWidth={1.5} fill="none" />
    </svg>
  );
}
