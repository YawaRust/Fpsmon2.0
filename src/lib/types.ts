/* ------------------------------------------------------------------
 * Модель данных оверлея — yawaside FPS Overlay
 *
 * Layout = блоки по оборудованию → элементы с собственным именем,
 * порядком и флагом sameLine. Плюс темы, хоткеи, автозапуск и
 * режим захвата сторонними программами (OBS).
 * ------------------------------------------------------------------ */

export type HW = 'fps' | 'cpu' | 'gpu' | 'ram' | 'io';

export type SensorId =
  // кадры
  | 'FPS' | 'FPSNative' | 'FPSGen' | 'FrameGenX' | 'Latency'
  | 'FPSAvg' | 'FPS1Low' | 'Frametime'
  // процессор
  | 'CPUUsage' | 'CPUTemp' | 'CPUClock' | 'CPUPower' | 'CPUFan'
  // видеокарта
  | 'GPUUsage' | 'GPUTemp' | 'GPUClock' | 'GPUPower' | 'GPUFan' | 'VRAM' | 'VRAMPct'
  // память
  | 'RAMUsage' | 'RAMUsed' | 'RAMFree'
  // диск и сеть
  | 'DiskRead' | 'DiskWrite' | 'DiskActive' | 'NetDown' | 'NetUp';

export interface SensorMeta {
  id: SensorId;
  name: string;
  unit: string;
  color: string;
  warning: number;
  critical: number;
  warnDir: 'high' | 'low';
  hw: HW;
  poll: 'fps' | 'sys';
  dec: number;
  /** датчик имеет смысл только при включённой генерации кадров */
  fgOnly?: boolean;
}

export const HW_META: Record<HW, { label: string; color: string }> = {
  fps: { label: 'Кадры',       color: '#22ff66' },
  cpu: { label: 'Процессор',   color: '#4db3ff' },
  gpu: { label: 'Видеокарта',  color: '#b9a7ff' },
  ram: { label: 'Память',      color: '#ffcc4d' },
  io:  { label: 'Диск и сеть', color: '#6ef3d6' },
};

export const SENSORS: SensorMeta[] = [
  { id: 'FPS',       name: 'FPS',   unit: '',     color: '#22ff66', warning: 45, critical: 30, warnDir: 'low',  hw: 'fps', poll: 'fps', dec: 0 },
  { id: 'FPSNative', name: 'NAT',   unit: '',     color: '#7fe0ff', warning: 0,  critical: 0,  warnDir: 'low',  hw: 'fps', poll: 'fps', dec: 0 },
  { id: 'FPSGen',    name: 'GEN',   unit: '',     color: '#ff9dff', warning: 0,  critical: 0,  warnDir: 'high', hw: 'fps', poll: 'fps', dec: 0, fgOnly: true },
  { id: 'FrameGenX', name: 'FG',    unit: '×',    color: '#ff7ae0', warning: 0,  critical: 0,  warnDir: 'high', hw: 'fps', poll: 'fps', dec: 0, fgOnly: true },
  { id: 'Latency',   name: 'LAT',   unit: 'ms',   color: '#ffd23f', warning: 35, critical: 55, warnDir: 'high', hw: 'fps', poll: 'fps', dec: 0 },
  { id: 'FPSAvg',    name: 'AVG',   unit: '',     color: '#8bffaf', warning: 0,  critical: 0,  warnDir: 'low',  hw: 'fps', poll: 'fps', dec: 0 },
  { id: 'FPS1Low',   name: '1%',    unit: '',     color: '#ffd23f', warning: 40, critical: 25, warnDir: 'low',  hw: 'fps', poll: 'fps', dec: 0 },
  { id: 'Frametime', name: 'FT',    unit: 'ms',   color: '#80ff80', warning: 20, critical: 33, warnDir: 'high', hw: 'fps', poll: 'fps', dec: 1 },

  { id: 'CPUUsage',  name: 'LOAD',  unit: '%',    color: '#4db3ff', warning: 80, critical: 95, warnDir: 'high', hw: 'cpu', poll: 'sys', dec: 0 },
  { id: 'CPUTemp',   name: 'TEMP',  unit: '°C',   color: '#ff9955', warning: 75, critical: 90, warnDir: 'high', hw: 'cpu', poll: 'sys', dec: 0 },
  { id: 'CPUClock',  name: 'CLK',   unit: 'MHz',  color: '#9ecbff', warning: 0,  critical: 0,  warnDir: 'high', hw: 'cpu', poll: 'sys', dec: 0 },
  { id: 'CPUPower',  name: 'PWR',   unit: 'W',    color: '#7fd4ff', warning: 0,  critical: 0,  warnDir: 'high', hw: 'cpu', poll: 'sys', dec: 0 },
  { id: 'CPUFan',    name: 'FAN',   unit: 'RPM',  color: '#67a9d6', warning: 0,  critical: 0,  warnDir: 'high', hw: 'cpu', poll: 'sys', dec: 0 },

  { id: 'GPUUsage',  name: 'LOAD',  unit: '%',    color: '#4dffb8', warning: 0,  critical: 0,  warnDir: 'high', hw: 'gpu', poll: 'sys', dec: 0 },
  { id: 'GPUTemp',   name: 'TEMP',  unit: '°C',   color: '#ff8080', warning: 80, critical: 90, warnDir: 'high', hw: 'gpu', poll: 'sys', dec: 0 },
  { id: 'GPUClock',  name: 'CLK',   unit: 'MHz',  color: '#b9a7ff', warning: 0,  critical: 0,  warnDir: 'high', hw: 'gpu', poll: 'sys', dec: 0 },
  { id: 'GPUPower',  name: 'PWR',   unit: 'W',    color: '#c9b6ff', warning: 0,  critical: 0,  warnDir: 'high', hw: 'gpu', poll: 'sys', dec: 0 },
  { id: 'GPUFan',    name: 'FAN',   unit: 'RPM',  color: '#9f8fd6', warning: 0,  critical: 0,  warnDir: 'high', hw: 'gpu', poll: 'sys', dec: 0 },
  { id: 'VRAM',      name: 'VRAM',  unit: 'MB',   color: '#cc80ff', warning: 0,  critical: 0,  warnDir: 'high', hw: 'gpu', poll: 'sys', dec: 0 },
  { id: 'VRAMPct',   name: 'VRAM',  unit: '%',    color: '#d9a0ff', warning: 85, critical: 95, warnDir: 'high', hw: 'gpu', poll: 'sys', dec: 0 },

  { id: 'RAMUsage',  name: 'LOAD',  unit: '%',    color: '#ffcc4d', warning: 85, critical: 95, warnDir: 'high', hw: 'ram', poll: 'sys', dec: 0 },
  { id: 'RAMUsed',   name: 'USED',  unit: 'MB',   color: '#e8d44d', warning: 0,  critical: 0,  warnDir: 'high', hw: 'ram', poll: 'sys', dec: 0 },
  { id: 'RAMFree',   name: 'FREE',  unit: 'MB',   color: '#d6c86b', warning: 0,  critical: 0,  warnDir: 'high', hw: 'ram', poll: 'sys', dec: 0 },

  { id: 'DiskRead',  name: 'READ',  unit: 'MB/s', color: '#7ee8fa', warning: 0,  critical: 0,  warnDir: 'high', hw: 'io',  poll: 'sys', dec: 1 },
  { id: 'DiskWrite', name: 'WRITE', unit: 'MB/s', color: '#62c7ec', warning: 0,  critical: 0,  warnDir: 'high', hw: 'io',  poll: 'sys', dec: 1 },
  { id: 'DiskActive',name: 'DISK',  unit: '%',    color: '#5bb8d9', warning: 0,  critical: 0,  warnDir: 'high', hw: 'io',  poll: 'sys', dec: 0 },
  { id: 'NetDown',   name: 'DOWN',  unit: 'KB/s', color: '#6ef3d6', warning: 0,  critical: 0,  warnDir: 'high', hw: 'io',  poll: 'sys', dec: 0 },
  { id: 'NetUp',     name: 'UP',    unit: 'KB/s', color: '#4fd6aa', warning: 0,  critical: 0,  warnDir: 'high', hw: 'io',  poll: 'sys', dec: 0 },
];

export const SENSOR_MAP: Record<SensorId, SensorMeta> = Object.fromEntries(
  SENSORS.map((s) => [s.id, s]),
) as Record<SensorId, SensorMeta>;

/* ---------------- layout ---------------- */

export interface Item {
  uid: string;
  sensor: SensorId;
  label: string;
  showLabel: boolean;
  color: string;
  warning: number;
  critical: number;
  showGraph: boolean;
  showUnit: boolean;
  sameLine: boolean;
  big: boolean;
  /** для FPS: дописывать «(нативные×множитель)» при включённом фреймгене */
  fgBreakdown?: boolean;
}

export interface Block {
  id: string;
  title: string;
  showTitle: boolean;
  enabled: boolean;
  hw: HW;
  items: Item[];
}

/* ---------------- hotkeys ---------------- */

export type HotkeyAction =
  | 'toggleOverlay' | 'toggleConfig' | 'cycleTheme' | 'resetStats' | 'toggleCapture' | 'cyclePosition';

export interface Hotkey {
  ctrl: boolean;
  shift: boolean;
  alt: boolean;
  /** код клавиши в формате KeyboardEvent.code */
  code: string;
}

export const HOTKEY_ACTIONS: { id: HotkeyAction; label: string; hint: string }[] = [
  { id: 'toggleOverlay', label: 'Показать / скрыть оверлей', hint: 'ShowWindow' },
  { id: 'toggleConfig',  label: 'Редактор настроек в игре', hint: 'in-overlay config' },
  { id: 'cycleTheme',    label: 'Следующая тема оформления', hint: 'cycle presets' },
  { id: 'cyclePosition', label: 'Следующий угол экрана', hint: 'cycle corners' },
  { id: 'resetStats',    label: 'Сбросить AVG / 1% LOW', hint: 'reset counters' },
  { id: 'toggleCapture', label: 'Захват сторонними программами', hint: 'WDA affinity' },
];

export function hk(code: string, mods: Partial<Hotkey> = {}): Hotkey {
  return { ctrl: false, shift: false, alt: false, code, ...mods };
}

export function hotkeyLabel(h: Hotkey): string {
  const parts: string[] = [];
  if (h.ctrl) parts.push('Ctrl');
  if (h.shift) parts.push('Shift');
  if (h.alt) parts.push('Alt');
  let key = h.code;
  if (key.startsWith('Key')) key = key.slice(3);
  else if (key.startsWith('Digit')) key = key.slice(5);
  else if (key.startsWith('Numpad')) key = 'Num' + key.slice(6);
  else if (key === 'ArrowUp') key = '↑';
  else if (key === 'ArrowDown') key = '↓';
  else if (key === 'ArrowLeft') key = '←';
  else if (key === 'ArrowRight') key = '→';
  parts.push(key);
  return parts.join(' + ');
}

export function hotkeyEquals(a: Hotkey, b: Hotkey): boolean {
  return a.code === b.code && a.ctrl === b.ctrl && a.shift === b.shift && a.alt === b.alt;
}

export function hotkeyMatches(h: Hotkey, e: KeyboardEvent): boolean {
  return e.code === h.code && e.ctrlKey === h.ctrl && e.shiftKey === h.shift && e.altKey === h.alt;
}

/** MOD_* флаги WinAPI для RegisterHotKey */
export function winMods(h: Hotkey): number {
  return (h.alt ? 1 : 0) | (h.ctrl ? 2 : 0) | (h.shift ? 4 : 0);
}

/* ---------------- settings ---------------- */

export type OverlayPos = 'tl' | 'tr' | 'bl' | 'br' | 'custom';
export type TitleStyle = 'plain' | 'chip' | 'bar';
export type CaptureMode = 'exclude' | 'allow';

export interface OverlaySettings {
  /* позиция */
  position: OverlayPos;
  customX: number;
  customY: number;

  /* тема / оформление */
  theme: string;
  fontFamily: string;
  fontSize: number;
  fontBold: boolean;
  opacity: number;
  bgColor: string;
  bgOpacity: number;
  borderColor: string;
  showBorder: boolean;
  rounded: boolean;
  cornerRadius: number;
  labelColor: string;
  accentColor: string;
  uppercase: boolean;
  letterSpacing: number;
  titleStyle: TitleStyle;
  padding: number;
  rowGap: number;
  blockGap: number;
  direction: 'column' | 'row';
  showSeparators: boolean;
  shadow: boolean;

  /* опрос */
  updateInterval: number;
  fpsInterval: number;

  /* окно */
  clickThrough: boolean;
  alwaysOnTop: boolean;

  /* захват сторонними программами (OBS) */
  captureMode: CaptureMode;

  /* автозапуск */
  autostart: boolean;
  autostartMethod: 'registry' | 'task';
  autostartDelay: number;
  autostartMinimized: boolean;

  /* фреймген */
  fgDetect: boolean;

  hotkeys: Record<HotkeyAction, Hotkey>;
  blocks: Block[];
}

export const WARNING_COLOR = '#ffd23f';
export const CRITICAL_COLOR = '#ff3b5c';

/* ---------------- factories ---------------- */

let _uid = 0;
export const uid = (p = 'i') => `${p}${(++_uid).toString(36)}${Math.random().toString(36).slice(2, 5)}`;

export function makeItem(sensor: SensorId, over: Partial<Item> = {}): Item {
  const m = SENSOR_MAP[sensor];
  return {
    uid: uid('it'),
    sensor,
    label: m.name,
    showLabel: true,
    color: m.color,
    warning: m.warning,
    critical: m.critical,
    showGraph: false,
    showUnit: true,
    sameLine: false,
    big: false,
    ...over,
  };
}

export function makeBlock(hw: HW, title: string, items: Item[], over: Partial<Block> = {}): Block {
  return { id: uid('bl'), title, showTitle: title.length > 0, enabled: true, hw, items, ...over };
}

export function defaultBlocks(): Block[] {
  return [
    makeBlock('fps', '', [
      makeItem('FPS', { label: '', showLabel: false, big: true, showGraph: true, fgBreakdown: true }),
      makeItem('FPS1Low', { label: '1%', sameLine: true }),
      makeItem('Latency', { label: 'LAT', sameLine: true }),
      makeItem('Frametime', { label: 'FT' }),
    ], { showTitle: false }),

    makeBlock('cpu', 'CPU', [
      makeItem('CPUUsage', { label: '', showLabel: false, showGraph: true }),
      makeItem('CPUTemp', { label: '', showLabel: false, sameLine: true }),
      makeItem('CPUClock', { label: 'CLK' }),
    ]),

    makeBlock('gpu', 'GPU', [
      makeItem('GPUUsage', { label: '', showLabel: false, showGraph: true }),
      makeItem('GPUTemp', { label: '', showLabel: false, sameLine: true }),
      makeItem('VRAM', { label: 'VRAM' }),
    ]),

    makeBlock('ram', 'RAM', [
      makeItem('RAMUsage', { label: '', showLabel: false }),
      makeItem('RAMUsed', { label: '', showLabel: false, sameLine: true }),
    ]),

    makeBlock('io', 'I/O', [
      makeItem('DiskRead', { label: 'R' }),
      makeItem('DiskWrite', { label: 'W', sameLine: true }),
      makeItem('NetDown', { label: '↓' }),
      makeItem('NetUp', { label: '↑', sameLine: true }),
    ], { enabled: false }),
  ];
}

export function defaultHotkeys(): Record<HotkeyAction, Hotkey> {
  return {
    toggleOverlay: hk('F12', { shift: true }),
    toggleConfig: hk('F11', { shift: true }),
    cycleTheme: hk('F10', { shift: true }),
    cyclePosition: hk('F9', { shift: true }),
    resetStats: hk('F8', { shift: true }),
    toggleCapture: hk('F7', { shift: true }),
  };
}

export function defaultSettings(): OverlaySettings {
  return {
    position: 'tl',
    customX: 14,
    customY: 14,

    theme: 'neon',
    fontFamily: '"JetBrains Mono", ui-monospace, monospace',
    fontSize: 14,
    fontBold: false,
    opacity: 0.96,
    bgColor: '#05070b',
    bgOpacity: 0.72,
    borderColor: '#94a3b8',
    showBorder: true,
    rounded: true,
    cornerRadius: 10,
    labelColor: '#cbd5e1',
    accentColor: '#00ff88',
    uppercase: true,
    letterSpacing: 0.06,
    titleStyle: 'plain',
    padding: 10,
    rowGap: 2,
    blockGap: 7,
    direction: 'column',
    showSeparators: true,
    shadow: true,

    updateInterval: 1000,
    fpsInterval: 500,

    clickThrough: false,
    alwaysOnTop: true,

    captureMode: 'allow',

    autostart: false,
    autostartMethod: 'registry',
    autostartDelay: 0,
    autostartMinimized: true,

    fgDetect: true,

    hotkeys: defaultHotkeys(),
    blocks: defaultBlocks(),
  };
}

/* ---------------- helpers ---------------- */

export function fmtValue(v: number, dec: number): string {
  if (v >= 10000) return Math.round(v).toLocaleString('ru-RU');
  return v.toFixed(dec);
}

export function resolveColor(item: Item, meta: SensorMeta, value: number): string {
  if (meta.warnDir === 'low') {
    if (item.critical > 0 && value <= item.critical) return CRITICAL_COLOR;
    if (item.warning > 0 && value <= item.warning) return WARNING_COLOR;
    return item.color;
  }
  if (item.critical > 0 && value >= item.critical) return CRITICAL_COLOR;
  if (item.warning > 0 && value >= item.warning) return WARNING_COLOR;
  return item.color;
}

export function toLines(items: Item[]): Item[][] {
  const lines: Item[][] = [];
  for (const it of items) {
    if (!it.sameLine || lines.length === 0) lines.push([it]);
    else lines[lines.length - 1].push(it);
  }
  return lines;
}

export function moveIn<T>(arr: T[], index: number, delta: number): T[] {
  const to = index + delta;
  if (to < 0 || to >= arr.length) return arr;
  const next = arr.slice();
  const [x] = next.splice(index, 1);
  next.splice(to, 0, x);
  return next;
}

export function hexToRgba(hex: string, a: number): string {
  const h = hex.replace('#', '');
  const r = parseInt(h.slice(0, 2), 16);
  const g = parseInt(h.slice(2, 4), 16);
  const b = parseInt(h.slice(4, 6), 16);
  return `rgba(${r},${g},${b},${a})`;
}
