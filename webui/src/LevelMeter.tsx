// Mirrors Source/ui/LevelMeter.h
import { useEffect, useId, useState } from 'react';
import type { PluginBackend } from './backend.ts';
import { meterBallistics, TICK_MS, type MeterState } from './meter.ts';

export function LevelMeter({ backend, width, height }: { backend: PluginBackend; width: number; height: number }) {
  const id = useId();
  const [{ level, clipping }, setState] = useState<MeterState>({ level: 0, clipping: false });

  useEffect(() => {
    const tick = meterBallistics();
    let loudest = 0;
    const unsubscribe = backend.onPeak((peak) => (loudest = Math.max(loudest, peak)));
    const timer = setInterval(() => {
      const next = tick(loudest);
      setState((last) => (last.level === next.level && last.clipping === next.clipping ? last : next));
      loudest = 0;
    }, TICK_MS);
    return () => {
      unsubscribe();
      clearInterval(timer);
    };
  }, [backend]);

  const inner = { x: 3, y: 3, width: width - 6, height: height - 6 };
  const fill = Math.min(1, level) * inner.height;

  return (
    <svg width={width} height={height} aria-hidden>
      <defs>
        <linearGradient id={`${id}fill`} x1="0" y1="1" x2="0" y2="0">
          <stop offset="0" stopColor="var(--color-magenta)" />
          {level > 0.85 && <stop offset="0.15" stopColor="#ff0000" />}
          <stop offset="1" stopColor="var(--color-accent)" />
        </linearGradient>
      </defs>
      <rect x={0.5} y={0.5} width={width - 1} height={height - 1} rx={4} fill="var(--color-panel)" fillOpacity={0.8} stroke="var(--color-panel-border)" />
      {fill > 0 && (
        <>
          <rect x={inner.x} y={inner.y + inner.height - fill} width={inner.width} height={fill} rx={2} fill={`url(#${id}fill)`} />
          <rect x={inner.x - 1} y={inner.y + inner.height - fill - 1} width={inner.width + 2} height={fill + 2} rx={2} fill="var(--color-accent)" fillOpacity={0.3} />
        </>
      )}
      {clipping ? (
        <>
          <rect x={0} y={-1} width={width} height={5} rx={1} fill="#ff0000" fillOpacity={0.4} />
          <rect x={0} y={0} width={width} height={3} rx={1} fill="#ff0000" />
        </>
      ) : (
        <rect x={0} y={0} width={width} height={3} rx={1} fill="#3a0000" fillOpacity={0.5} />
      )}
    </svg>
  );
}
