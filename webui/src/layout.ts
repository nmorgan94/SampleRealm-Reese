// AudioPluginAudioProcessorEditor::resized(), worked through
export interface Rect {
  left: number;
  top: number;
  width: number;
  height: number;
}

// juce::Rectangle::withSizeKeepingCentre
export const centred = (outer: number, inner: number) => Math.trunc((outer - inner) / 2);

export const EDITOR_WIDTH = 625;
export const EDITOR_HEIGHT = 300;

const MARGIN = 3;
export const HEADER_HEIGHT = 40;

export const header = { x: MARGIN, y: MARGIN, width: EDITOR_WIDTH - 2 * MARGIN, height: HEADER_HEIGHT };
export const panel = { x: MARGIN, y: MARGIN + HEADER_HEIGHT, width: header.width, height: EDITOR_HEIGHT - 2 * MARGIN - HEADER_HEIGHT };

const area = { x: panel.x + 12, y: panel.y + 12, width: panel.width - 24, height: panel.height - 24 };
const COLUMNS = 8;
const columnWidth = Math.floor(area.width / COLUMNS);
const rowHeight = Math.floor(area.height / 2);

const LABEL_SPACE = 24;
// Orbitron 12 plus the label's border, placed above the slider by attachToComponent
const LABEL_HEIGHT = 14;

export const cell = (index: number) => {
  const x = area.x + (index % COLUMNS) * columnWidth;
  const y = area.y + Math.floor(index / COLUMNS) * rowHeight;
  const slider = { x, y: y + LABEL_SPACE, width: columnWidth, height: rowHeight - LABEL_SPACE };
  return { slider, label: { x, y: slider.y - LABEL_HEIGHT, width: columnWidth, height: LABEL_HEIGHT } };
};

// On a slider's top edge, 40 in
export const toggleOn = (index: number) => ({ x: cell(index).slider.x + 40, y: cell(index).slider.y + 5, width: 30, height: 10 });

export const adsr = { x: area.x + 4 * columnWidth, y: area.y + rowHeight + 10, width: 4 * columnWidth, height: rowHeight - 15 };

const panelBottom = panel.y + panel.height;
export const transport = { x: panel.x + 15, y: panelBottom - 23, width: 140, height: 12 };
export const version = { x: panel.x + panel.width - 60, y: panelBottom - 23, width: 50, height: 12 };
