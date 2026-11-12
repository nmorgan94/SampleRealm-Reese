// Mirrors Source/ui/CustomLookAndFeel.h and TriangleSelectorLookAndFeel.h
import { useId } from 'react';
import { juceHeight } from '@samplerealm/modules/editor/fonts.ts';
import { useParam } from '@samplerealm/modules/editor/hooks.ts';
import { arcPath, ROTARY_END, ROTARY_START, type RotaryProps } from '@samplerealm/modules/editor/LabeledSlider.tsx';
import type { EditorBackend } from './backend.ts';

const ROTARY_SWEEP = ROTARY_END - ROTARY_START;

const CYAN = 'var(--color-accent)';
const MAGENTA = 'var(--color-magenta)';

const degrees = (radians: number) => (radians * 180) / Math.PI;

// The slider's bounds less 10 on each side, as drawRotarySlider
const knobGeometry = (width: number, height: number) => {
  const radius = Math.min(width - 20, height - 20) / 2;
  return { cx: width / 2, cy: height / 2, radius };
};

// CustomLookAndFeel::drawRotarySlider
export function KnobRotary({ width, height, proportion }: RotaryProps) {
  const id = useId();
  const { cx, cy, radius } = knobGeometry(width, height);
  const angle = ROTARY_START + proportion * ROTARY_SWEEP;
  const lineWidth = Math.min(8, radius * 0.5);
  const arcRadius = radius - lineWidth / 2;
  const knob = radius * 0.8;
  const pointerLength = radius * 0.5;

  return (
    <svg width={width} height={height} aria-hidden>
      <defs>
        <linearGradient id={`${id}arc`} gradientUnits="userSpaceOnUse" x1={cx - arcRadius} y1={cy} x2={cx + arcRadius} y2={cy}>
          <stop offset="0" stopColor={CYAN} />
          <stop offset="1" stopColor={MAGENTA} />
        </linearGradient>
        <linearGradient id={`${id}knob`} gradientUnits="userSpaceOnUse" x1={cx} y1={cy - knob} x2={cx} y2={cy + knob}>
          <stop offset="0" stopColor="#3a4f6f" />
          <stop offset="1" stopColor="#1a2f4f" />
        </linearGradient>
      </defs>
      {proportion > 0 && (
        <>
          <path d={arcPath(cx, cy, arcRadius, ROTARY_START, angle)} stroke={`url(#${id}arc)`} strokeWidth={lineWidth} strokeLinecap="round" fill="none" />
          <path d={arcPath(cx, cy, arcRadius, ROTARY_START, angle)} stroke={CYAN} strokeOpacity={0.3} strokeWidth={lineWidth + 0.5} strokeLinecap="round" fill="none" />
        </>
      )}
      <circle cx={cx} cy={cy} r={knob} fill="var(--color-panel-border)" />
      <circle cx={cx} cy={cy} r={knob * 0.9} fill={`url(#${id}knob)`} />
      <circle cx={cx - knob * 0.2} cy={cy - knob * 0.4} r={knob * 0.4} fill="#5a7f9f" fillOpacity={0.6} />
      <g transform={`translate(${cx} ${cy}) rotate(${degrees(angle)})`}>
        <rect x={-1.5} y={-pointerLength} width={3} height={pointerLength * 0.6} fill={CYAN} fillOpacity={0.5} />
        <rect x={-0.9} y={-pointerLength} width={1.8} height={pointerLength * 0.6} fill={CYAN} />
      </g>
      <circle cx={cx} cy={cy} r={2} fill={CYAN} />
    </svg>
  );
}

const SYMBOL_SIZE = 8.5;

// Square, saw and triangle, at 150°, 270° and 30° from 3 o'clock
const symbolPaths = (cx: number, cy: number, radius: number) => {
  const half = SYMBOL_SIZE / 2;
  const low = SYMBOL_SIZE * 0.4;
  const tall = SYMBOL_SIZE * 0.5;

  return [
    (x: number, y: number) => `M ${x - half} ${y + low} L ${x - half} ${y - low} L ${x + half} ${y - low} L ${x + half} ${y + low}`,
    (x: number, y: number) => `M ${x - half} ${y + low} L ${x} ${y - low} L ${x} ${y + low} L ${x + half} ${y - low} L ${x + half} ${y + low}`,
    (x: number, y: number) => `M ${x - half} ${y + tall} L ${x} ${y - tall} L ${x + half} ${y + tall}`,
  ].map((path, i) => {
    const angle = (5 * Math.PI) / 6 + (i * 2 * Math.PI) / 3;
    return path(cx + Math.cos(angle) * radius * 1.25, cy + Math.sin(angle) * radius * 1.25);
  });
};

// TriangleSelectorLookAndFeel::drawRotarySlider
export function TriangleRotary({ width, height, value }: RotaryProps) {
  const { cx, cy, radius } = knobGeometry(width, height);
  const size = radius * 0.8;

  return (
    <svg width={width} height={height} aria-hidden>
      <g transform={`translate(${cx} ${cy}) rotate(${degrees(((value - 1) * 2 * Math.PI) / 3)})`}>
        <path d={`M 0 ${-size} L ${-size * 0.6} ${size * 0.5} L ${size * 0.6} ${size * 0.5} Z`} fill={CYAN} fillOpacity={0.3} stroke={CYAN} strokeWidth={2} />
      </g>
      <circle cx={cx} cy={cy} r={2} fill={CYAN} />
      {symbolPaths(cx, cy, radius).map((d) => (
        <path key={d} d={d} stroke={CYAN} strokeOpacity={0.5} strokeWidth={1.2} fill="none" />
      ))}
    </svg>
  );
}

// CustomLookAndFeel::drawToggleButton, for an Off / On parameter
export function SmallToggle({ backend, id, text }: { backend: EditorBackend; id: string; text: (on: boolean) => string }) {
  const { value, commit, param } = useParam(backend, id);
  const on = value >= 0.5;

  return (
    <button
      type="button"
      className={`size-full cursor-pointer rounded-[3px] border font-bold hover:brightness-125 ${
        on ? 'border-accent bg-linear-to-b from-accent/30 to-accent/15 text-accent' : 'border-panel-border bg-linear-to-b from-panel to-panel-dark text-fg/60'
      }`}
      style={{ fontSize: juceHeight(9), lineHeight: 1 }}
      aria-label={param.name}
      aria-pressed={on}
      onClick={() => commit(on ? 0 : 1)}
      onDoubleClick={(e) => e.stopPropagation()}
    >
      {text(on)}
    </button>
  );
}
