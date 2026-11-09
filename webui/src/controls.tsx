// Mirrors Source/ui/CustomLookAndFeel.h, TriangleSelectorLookAndFeel.h and juce::Slider's rotary drag
import {
  useId,
  useRef,
  type CSSProperties,
  type MouseEvent as ReactMouseEvent,
  type PointerEvent as ReactPointerEvent,
} from 'react';
import type { PluginBackend } from './backend.ts';
import type { Rect } from './layout.ts';
import { juceHeight } from './fonts.ts';
import { useParam, type ParamState } from './hooks.ts';
import { fromProportion, toProportion } from './params.ts';

// JUCE's default rotary angles, clockwise from 12 o'clock
const ROTARY_START = 1.2 * Math.PI;
const ROTARY_SWEEP = 1.6 * Math.PI;

export type { Rect };

const CYAN = 'var(--color-accent)';
const MAGENTA = 'var(--color-magenta)';

const pointAt = (cx: number, cy: number, radius: number, angle: number) =>
  [cx + radius * Math.sin(angle), cy - radius * Math.cos(angle)] as const;

const arcPath = (cx: number, cy: number, radius: number, from: number, to: number) => {
  const [x1, y1] = pointAt(cx, cy, radius, from);
  const [x2, y2] = pointAt(cx, cy, radius, to);
  return `M ${x1} ${y1} A ${radius} ${radius} 0 ${to - from > Math.PI ? 1 : 0} 1 ${x2} ${y2}`;
};

const degrees = (radians: number) => (radians * 180) / Math.PI;

// The slider's bounds less 10 on each side, as drawRotarySlider
const knobGeometry = (width: number, height: number) => {
  const radius = Math.min(width - 20, height - 20) / 2;
  return { cx: width / 2, cy: height / 2, radius };
};

function KnobGraphic({ width, height, proportion }: { width: number; height: number; proportion: number }) {
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

function TriangleGraphic({ width, height, value }: { width: number; height: number; value: number }) {
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

type DragStart = { x: number; y: number; proportion: number };

interface KnobProps {
  backend: PluginBackend;
  state: ParamState;
  width: number;
  height: number;
  // juce::Slider::setMouseDragSensitivity
  pixelsForFullDrag: number;
  look: 'rotary' | 'triangle';
  onDragStart: () => void;
  onDragEnd: () => void;
}

export function Knob({ backend, state, width, height, pixelsForFullDrag, look, onDragStart, onDragEnd }: KnobProps) {
  const drag = useRef<DragStart | null>(null);
  const { value, set, commit, param } = state;
  const { id } = param;

  const end = () => {
    if (!drag.current) return;
    drag.current = null;
    backend.endGesture(id);
    onDragEnd();
  };

  const follow = (e: ReactPointerEvent) => {
    if (!drag.current) return;
    const moved = e.clientX - drag.current.x + (drag.current.y - e.clientY);
    const next = fromProportion(param, drag.current.proportion + moved / pixelsForFullDrag);
    if (next !== value) set(next);
  };

  return (
    <div
      className="cursor-pointer touch-none"
      role="slider"
      aria-label={param.name}
      aria-valuemin={param.min}
      aria-valuemax={param.max}
      aria-valuenow={value}
      style={{ width, height }}
      onPointerDown={(e) => {
        if (e.button !== 0) return;
        e.currentTarget.setPointerCapture(e.pointerId);
        drag.current = { x: e.clientX, y: e.clientY, proportion: toProportion(param, value) };
        backend.beginGesture(id);
        onDragStart();
      }}
      onPointerMove={follow}
      onPointerUp={end}
      onPointerCancel={end}
      // SliderAttachment returns a double-click to the default
      onDoubleClick={() => commit(param.defaultValue)}
    >
      {look === 'triangle' ? (
        <TriangleGraphic width={width} height={height} value={value} />
      ) : (
        <KnobGraphic width={width} height={height} proportion={toProportion(param, value)} />
      )}
    </div>
  );
}

// CustomLookAndFeel::drawToggleButton, for an Off / On parameter
export function SmallToggle({ backend, id, text }: { backend: PluginBackend; id: string; text: (on: boolean) => string }) {
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

interface TextButtonProps {
  text: string;
  style: CSSProperties;
  onClick(e: ReactMouseEvent<HTMLButtonElement>): void;
}

// LookAndFeel_V4::drawButtonBackground lightens by 5% on hover and 20% while down
export function TextButton({ text, style, onClick }: TextButtonProps) {
  return (
    <button
      type="button"
      className="absolute cursor-pointer overflow-hidden rounded-md border border-line bg-widget px-1 text-ellipsis whitespace-nowrap text-accent hover:bg-[color-mix(in_srgb,var(--color-widget),white_5%)] active:bg-[color-mix(in_srgb,var(--color-widget),white_20%)]"
      style={{ fontSize: juceHeight(12), ...style }}
      onClick={onClick}
    >
      {text}
    </button>
  );
}
