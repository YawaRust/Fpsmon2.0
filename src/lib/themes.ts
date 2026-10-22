/* ------------------------------------------------------------------
 * Готовые стили оверлея. Тема патчит только визуальные поля —
 * раскладка блоков, хоткеи и опрос датчиков не затрагиваются.
 * ------------------------------------------------------------------ */

import type { OverlaySettings, TitleStyle } from './types';

export interface ThemeStyle {
  fontFamily: string;
  fontSize: number;
  fontBold: boolean;
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
  showSeparators: boolean;
  shadow: boolean;
  /** перекраска значений: цвет на устройство */
  palette: { fps: string; cpu: string; gpu: string; ram: string; io: string };
}

export interface ThemePreset {
  id: string;
  name: string;
  desc: string;
  style: ThemeStyle;
}

const MONO = '"JetBrains Mono", ui-monospace, monospace';
const SANS = 'Manrope, "Segoe UI", system-ui, sans-serif';
const DISPLAY = 'Unbounded, Impact, sans-serif';

export const THEMES: ThemePreset[] = [
  {
    id: 'neon',
    name: 'Neon Grid',
    desc: 'фирменный тёмный с неоново-зелёным акцентом',
    style: {
      fontFamily: MONO, fontSize: 14, fontBold: false,
      bgColor: '#05070b', bgOpacity: 0.72, borderColor: '#94a3b8', showBorder: true,
      rounded: true, cornerRadius: 10, labelColor: '#cbd5e1', accentColor: '#00ff88',
      uppercase: true, letterSpacing: 0.06, titleStyle: 'plain',
      padding: 10, rowGap: 2, blockGap: 7, showSeparators: true, shadow: true,
      palette: { fps: '#22ff66', cpu: '#4db3ff', gpu: '#b9a7ff', ram: '#ffcc4d', io: '#6ef3d6' },
    },
  },
  {
    id: 'rtss',
    name: 'RTSS Classic',
    desc: 'без подложки, крупный моноширинный — как RivaTuner',
    style: {
      fontFamily: MONO, fontSize: 16, fontBold: true,
      bgColor: '#000000', bgOpacity: 0, borderColor: '#000000', showBorder: false,
      rounded: false, cornerRadius: 0, labelColor: '#9fe8b4', accentColor: '#44ff77',
      uppercase: true, letterSpacing: 0.02, titleStyle: 'plain',
      padding: 4, rowGap: 0, blockGap: 3, showSeparators: false, shadow: false,
      palette: { fps: '#44ff77', cpu: '#44ff77', gpu: '#44ff77', ram: '#44ff77', io: '#44ff77' },
    },
  },
  {
    id: 'glass',
    name: 'Frosted Glass',
    desc: 'светлое матовое стекло, мягкие тени',
    style: {
      fontFamily: SANS, fontSize: 14, fontBold: true,
      bgColor: '#f3f6fb', bgOpacity: 0.16, borderColor: '#ffffff', showBorder: true,
      rounded: true, cornerRadius: 14, labelColor: '#dbe6f5', accentColor: '#ffffff',
      uppercase: false, letterSpacing: 0.01, titleStyle: 'chip',
      padding: 12, rowGap: 4, blockGap: 9, showSeparators: false, shadow: true,
      palette: { fps: '#ffffff', cpu: '#bcd9ff', gpu: '#e2c9ff', ram: '#ffe6ab', io: '#b6fff0' },
    },
  },
  {
    id: 'terminal',
    name: 'Terminal',
    desc: 'чёрный бокс, янтарный текст, рамка 1px',
    style: {
      fontFamily: MONO, fontSize: 13, fontBold: false,
      bgColor: '#000000', bgOpacity: 0.86, borderColor: '#ffb03a', showBorder: true,
      rounded: false, cornerRadius: 0, labelColor: '#9a6b22', accentColor: '#ffb03a',
      uppercase: true, letterSpacing: 0.12, titleStyle: 'bar',
      padding: 8, rowGap: 1, blockGap: 6, showSeparators: true, shadow: false,
      palette: { fps: '#ffb03a', cpu: '#ffc76b', gpu: '#ffa021', ram: '#e09020', io: '#d98020' },
    },
  },
  {
    id: 'cyber',
    name: 'Cyber Magenta',
    desc: 'киберпанк: пурпур, широкие трекинги',
    style: {
      fontFamily: DISPLAY, fontSize: 13, fontBold: true,
      bgColor: '#120418', bgOpacity: 0.78, borderColor: '#ff2bd1', showBorder: true,
      rounded: true, cornerRadius: 4, labelColor: '#c58ae0', accentColor: '#ff2bd1',
      uppercase: true, letterSpacing: 0.18, titleStyle: 'bar',
      padding: 11, rowGap: 3, blockGap: 8, showSeparators: true, shadow: true,
      palette: { fps: '#ff2bd1', cpu: '#00e5ff', gpu: '#b47aff', ram: '#ffd23f', io: '#6ef3d6' },
    },
  },
  {
    id: 'minimal',
    name: 'Minimal Mono',
    desc: 'белым по прозрачному, без рамок и фона',
    style: {
      fontFamily: SANS, fontSize: 14, fontBold: true,
      bgColor: '#000000', bgOpacity: 0.28, borderColor: '#ffffff', showBorder: false,
      rounded: true, cornerRadius: 8, labelColor: '#8f98a6', accentColor: '#ffffff',
      uppercase: false, letterSpacing: 0, titleStyle: 'plain',
      padding: 8, rowGap: 2, blockGap: 6, showSeparators: false, shadow: false,
      palette: { fps: '#ffffff', cpu: '#e2e8f0', gpu: '#e2e8f0', ram: '#e2e8f0', io: '#e2e8f0' },
    },
  },
  {
    id: 'esport',
    name: 'Esport Bar',
    desc: 'плотная горизонтальная полоска для турниров',
    style: {
      fontFamily: MONO, fontSize: 13, fontBold: true,
      bgColor: '#070b12', bgOpacity: 0.9, borderColor: '#1f2b3d', showBorder: true,
      rounded: true, cornerRadius: 6, labelColor: '#64748b', accentColor: '#38bdf8',
      uppercase: true, letterSpacing: 0.08, titleStyle: 'chip',
      padding: 7, rowGap: 0, blockGap: 10, showSeparators: true, shadow: true,
      palette: { fps: '#38bdf8', cpu: '#f472b6', gpu: '#a78bfa', ram: '#facc15', io: '#4ade80' },
    },
  },
  {
    id: 'crt',
    name: 'Retro CRT',
    desc: 'фосфорный зелёный ЭЛТ-монитор',
    style: {
      fontFamily: MONO, fontSize: 15, fontBold: false,
      bgColor: '#021206', bgOpacity: 0.82, borderColor: '#2dff6a', showBorder: true,
      rounded: true, cornerRadius: 3, labelColor: '#1f8f3f', accentColor: '#2dff6a',
      uppercase: true, letterSpacing: 0.14, titleStyle: 'bar',
      padding: 10, rowGap: 2, blockGap: 7, showSeparators: true, shadow: false,
      palette: { fps: '#2dff6a', cpu: '#55ff88', gpu: '#88ffaa', ram: '#aaffbb', io: '#2dff6a' },
    },
  },
];

export const THEME_MAP: Record<string, ThemePreset> = Object.fromEntries(
  THEMES.map((t) => [t.id, t]),
);

/** Применить тему: визуальные поля + перекраска значений по устройству */
export function applyTheme(s: OverlaySettings, themeId: string): OverlaySettings {
  const t = THEME_MAP[themeId];
  if (!t) return s;
  const st = t.style;
  return {
    ...s,
    theme: themeId,
    fontFamily: st.fontFamily,
    fontSize: st.fontSize,
    fontBold: st.fontBold,
    bgColor: st.bgColor,
    bgOpacity: st.bgOpacity,
    borderColor: st.borderColor,
    showBorder: st.showBorder,
    rounded: st.rounded,
    cornerRadius: st.cornerRadius,
    labelColor: st.labelColor,
    accentColor: st.accentColor,
    uppercase: st.uppercase,
    letterSpacing: st.letterSpacing,
    titleStyle: st.titleStyle,
    padding: st.padding,
    rowGap: st.rowGap,
    blockGap: st.blockGap,
    showSeparators: st.showSeparators,
    shadow: st.shadow,
    blocks: s.blocks.map((b) => ({
      ...b,
      items: b.items.map((it) => ({ ...it, color: st.palette[b.hw] })),
    })),
  };
}
