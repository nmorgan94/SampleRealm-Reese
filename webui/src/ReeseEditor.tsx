// Mirrors AudioPluginAudioProcessorEditor::paint() and resized()
import { memo, useEffect, useState } from 'react';
import { AdsrView } from './AdsrView.tsx';
import type { PluginBackend } from './backend.ts';
import { Knob, SmallToggle } from './controls.tsx';
import { EditorRootContext } from './editorRoot.ts';
import { juceHeight } from './fonts.ts';
import { HeaderBar, type HeaderLayout, type HeaderTitle } from './HeaderBar.tsx';
import { useParam } from './hooks.ts';
import { dragText, isWave } from './labels.ts';
import { adsr, cell, EDITOR_HEIGHT, EDITOR_WIDTH, header, HEADER_HEIGHT, panel, toggleOn, transport, version } from './layout.ts';
import { BAR_WIDTH, LevelMeter } from './LevelMeter.tsx';
import { useManual } from './ManualOverlay.tsx';
import { usePrompt } from './PromptOverlay.tsx';

// AudioPluginAudioProcessorEditor::title and headerLayout
const HEADER_TITLE: HeaderTitle = { bold: 'SampleRealm: ', regular: 'REESE' };
const HEADER_LAYOUT: HeaderLayout = {
  height: HEADER_HEIGHT,
  edge: 0,
  titleWidth: 340,
  titleHeight: 35.112,
  gap: 8,
  controlsHeight: 24,
  meterWidth: BAR_WIDTH,
  meterInset: 4,
};

// In the editor's ControlIndex order
const CONTROLS: { id: string; label: string }[] = [
  { id: 'oscAWave', label: 'OSC A' },
  { id: 'oscBWave', label: 'OSC B' },
  { id: 'sub', label: 'SUB' },
  { id: 'detune', label: 'DETUNE' },
  { id: 'glideTime', label: 'GLIDE' },
  { id: 'unisonVoices', label: 'UNISON' },
  { id: 'drive', label: 'DRIVE' },
  { id: 'output', label: 'OUTPUT' },
  { id: 'cutoff', label: 'CUTOFF' },
  { id: 'resonance', label: 'RESONANCE' },
  { id: 'lfoRate', label: 'LFO RATE' },
  { id: 'lfoDepth', label: 'LFO DEPTH' },
  { id: 'attack', label: 'ATTACK' },
  { id: 'decay', label: 'DECAY' },
  { id: 'sustain', label: 'SUSTAIN' },
  { id: 'release', label: 'RELEASE' },
];

const indexOf = (id: string) => CONTROLS.findIndex((control) => control.id === id);
const CUTOFF = indexOf('cutoff');
const LFO_RATE = indexOf('lfoRate');
const LFO_DEPTH = indexOf('lfoDepth');
const ATTACK = indexOf('attack');

const box = ({ x, y, width, height }: { x: number; y: number; width: number; height: number }) => ({ left: x, top: y, width, height });

function LabeledKnob({ backend, index, id, label }: { backend: PluginBackend; index: number; id: string; label: string }) {
  const [dragging, setDragging] = useState(false);
  const state = useParam(backend, id);
  const { slider, label: labelBox } = cell(index);

  return (
    <>
      <div
        className="absolute flex items-center justify-center font-bold whitespace-nowrap"
        style={{ ...box(labelBox), fontSize: juceHeight(12) }}
      >
        {dragging ? dragText(state.param, state.value) : label}
      </div>
      <div className="absolute" style={box(slider)}>
        <Knob
          backend={backend}
          state={state}
          width={slider.width}
          height={slider.height}
          look={isWave(id) ? 'triangle' : 'rotary'}
          pixelsForFullDrag={isWave(id) ? 75 : 150}
          onDragStart={() => setDragging(true)}
          onDragEnd={() => setDragging(false)}
        />
      </div>
    </>
  );
}

function Panel() {
  const { x, y, width, height } = panel;
  const right = x + width;
  const bottom = y + height;
  const dividerY = y + height / 2;
  const depth = cell(LFO_DEPTH).slider;
  const between = (depth.x + depth.width + cell(ATTACK).slider.x) / 2;
  const inset = 8;
  const corners = [
    `M ${x + inset} ${y + inset + 15} V ${y + inset} H ${x + inset + 15}`,
    `M ${right - inset} ${y + inset + 15} V ${y + inset} H ${right - inset - 15}`,
    `M ${x + inset} ${bottom - inset - 15} V ${bottom - inset} H ${x + inset + 15}`,
    `M ${right - inset} ${bottom - inset - 15} V ${bottom - inset} H ${right - inset - 15}`,
  ];

  return (
    <svg className="pointer-events-none absolute inset-0" width={EDITOR_WIDTH} height={EDITOR_HEIGHT} aria-hidden>
      <defs>
        <linearGradient id="panel" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stopColor="var(--color-panel)" stopOpacity={0.6} />
          <stop offset="1" stopColor="var(--color-panel-dark)" stopOpacity={0.8} />
        </linearGradient>
        <linearGradient id="border" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0" stopColor="#2a3f5f" />
          <stop offset="1" stopColor="#1a2f4f" />
        </linearGradient>
      </defs>
      <rect x={x - 2} y={y - 2} width={width + 4} height={height + 4} rx={14} fill="var(--color-accent)" fillOpacity={0.08} />
      <rect x={x} y={y} width={width} height={height} rx={12} fill="url(#panel)" />
      <rect x={x} y={y} width={width} height={height} rx={12} fill="none" stroke="url(#border)" strokeWidth={2} />
      <rect x={x + 2} y={y + 2} width={width - 4} height={height - 4} rx={10} fill="none" stroke="#3a4f6f" strokeOpacity={0.3} />
      <line x1={x + 20} y1={dividerY} x2={right - 20} y2={dividerY} stroke="var(--color-accent)" strokeOpacity={0.1} strokeWidth={2} />
      <line x1={x + 20} y1={dividerY} x2={right - 20} y2={dividerY} stroke="#2a3f5f" strokeOpacity={0.5} />
      <line x1={between} y1={depth.y + 15} x2={between} y2={depth.y + depth.height - 15} stroke="var(--color-accent)" strokeOpacity={0.1} strokeWidth={2} />
      <line x1={between} y1={depth.y + 15} x2={between} y2={depth.y + depth.height - 15} stroke="#2a3f5f" strokeOpacity={0.5} />
      {corners.map((d) => (
        <path key={d} d={d} stroke="var(--color-accent)" strokeOpacity={0.4} strokeWidth={2} fill="none" />
      ))}
    </svg>
  );
}

function Bpm({ backend }: { backend: PluginBackend }) {
  const [bpm, setBpm] = useState(0);
  useEffect(() => backend.onBpm(setBpm), [backend]);

  return (
    <div className="absolute flex items-center text-accent/40" style={{ ...box(transport), fontSize: 9 }}>
      {bpm > 0 ? `${bpm.toFixed(1)} BPM` : '-- BPM'}
    </div>
  );
}

export const ReeseEditor = memo(function ReeseEditor({ backend }: { backend: PluginBackend }) {
  const [root, setRoot] = useState<HTMLDivElement | null>(null);
  const sync = useParam(backend, 'lfoSync').value >= 0.5;
  const { prompt, overlay } = usePrompt();
  const manual = useManual();

  return (
    <EditorRootContext.Provider value={root}>
    <div
      ref={setRoot}
      className="relative overflow-hidden bg-linear-to-b from-background-top to-background select-none"
      style={{ width: EDITOR_WIDTH, height: EDITOR_HEIGHT }}
    >
      <Panel />
      <div className="absolute" style={box(adsr)}>
        <AdsrView backend={backend} width={adsr.width} height={adsr.height} />
      </div>

      {CONTROLS.map(({ id, label }, index) => {
        const synced = sync && index === LFO_RATE;
        const knobId = synced ? 'lfoSyncRate' : id;
        return <LabeledKnob key={knobId} backend={backend} index={index} id={knobId} label={synced ? 'LFO SYNC' : label} />;
      })}

      <div className="absolute" style={box(toggleOn(LFO_RATE))}>
        <SmallToggle backend={backend} id="lfoSync" text={() => 'SYNC'} />
      </div>
      <div className="absolute" style={box(toggleOn(CUTOFF))}>
        <SmallToggle backend={backend} id="filter24db" text={(on) => (on ? '24dB' : '12dB')} />
      </div>

      <Bpm backend={backend} />
      <div className="absolute flex items-center justify-end text-accent/40" style={{ ...box(version), fontSize: 9 }}>
        v{backend.info.version}
      </div>

      <div className="absolute" style={box(header)}>
        <HeaderBar
          title={HEADER_TITLE}
          width={header.width}
          layout={HEADER_LAYOUT}
          backend={backend}
          prompt={prompt}
          showManual={manual.showManual}
          meter={({ left, top, width, height }) => <LevelMeter backend={backend} x={left} y={top} width={width} height={height} channels={1} />}
        />
      </div>
      {overlay}
      {manual.overlay}
    </div>
    </EditorRootContext.Provider>
  );
});
