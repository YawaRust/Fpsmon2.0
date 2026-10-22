/* ------------------------------------------------------------------
 * Экспорт конфигурации в INI (формат Settings::save() нативной версии).
 * ------------------------------------------------------------------ */

import { HOTKEY_ACTIONS, SENSOR_MAP, hotkeyLabel, winMods, type OverlaySettings } from './types';

const POS_INDEX: Record<string, number> = { tl: 0, tr: 1, bl: 2, br: 3, custom: 4 };

function hexToRgbf(hex: string): [number, number, number] {
  const h = hex.replace('#', '');
  return [
    parseInt(h.slice(0, 2), 16) / 255,
    parseInt(h.slice(2, 4), 16) / 255,
    parseInt(h.slice(4, 6), 16) / 255,
  ];
}

/** грубое соответствие code → виртуальный код Windows */
function vkCode(code: string): number {
  if (/^F\d{1,2}$/.test(code)) return 0x6f + Number(code.slice(1));
  if (code.startsWith('Key')) return code.charCodeAt(3);
  if (code.startsWith('Digit')) return 0x30 + Number(code[5]);
  const map: Record<string, number> = {
    Insert: 0x2d, Delete: 0x2e, Home: 0x24, End: 0x23, PageUp: 0x21, PageDown: 0x22,
    ArrowUp: 0x26, ArrowDown: 0x28, ArrowLeft: 0x25, ArrowRight: 0x27, Space: 0x20,
    Backquote: 0xc0, Backslash: 0xdc, Enter: 0x0d, NumpadMultiply: 0x6a, NumpadAdd: 0x6b,
  };
  return map[code] ?? 0;
}

export function buildIni(s: OverlaySettings, brand = 'yawaside Overlay'): string {
  const L: string[] = [];
  const b = (v: boolean) => (v ? 1 : 0);

  L.push(`; ${brand} 0.0.1 — конфигурация`, '; сгенерировано встроенным редактором (in-overlay)', '');

  L.push('[General]');
  L.push('version=0.0.1');
  L.push(`position=${POS_INDEX[s.position] ?? 0}`);
  L.push(`customX=${s.customX}`);
  L.push(`customY=${s.customY}`);
  L.push(`direction=${s.direction === 'row' ? 1 : 0}   ; 0=column 1=row`);
  L.push(`updateInterval=${Math.round(s.updateInterval)}`);
  L.push(`fpsUpdateInterval=${Math.round(s.fpsInterval)}`);
  L.push(`alwaysOnTop=${b(s.alwaysOnTop)}`);
  L.push(`clickThrough=${b(s.clickThrough)}`);
  L.push(`frameGenDetect=${b(s.fgDetect)}   ; DLSS3 / FSR3 / XeSS-FG`, '');

  L.push('[Capture]');
  L.push(`; allow  = WDA_NONE — оверлей виден в OBS / Discord / скриншотах`);
  L.push(`; exclude= WDA_EXCLUDEFROMCAPTURE — невидим для захвата`);
  L.push(`mode=${s.captureMode}`);
  L.push(`displayAffinity=${s.captureMode === 'allow' ? 'WDA_NONE' : 'WDA_EXCLUDEFROMCAPTURE'}`, '');

  L.push('[Autostart]');
  L.push(`enabled=${b(s.autostart)}`);
  L.push(`method=${s.autostartMethod}   ; registry | task`);
  L.push(`delaySeconds=${s.autostartDelay}`);
  L.push(`startMinimized=${b(s.autostartMinimized)}`, '');

  L.push('[Appearance]');
  L.push(`theme=${s.theme}`);
  L.push(`fontFamily=${s.fontFamily.split(',')[0].replace(/"/g, '')}`);
  L.push(`fontSize=${s.fontSize}`);
  L.push(`fontBold=${b(s.fontBold)}`);
  L.push(`opacity=${s.opacity.toFixed(2)}`);
  L.push(`bgColor=${s.bgColor}`);
  L.push(`bgAlpha=${s.bgOpacity.toFixed(2)}`);
  L.push(`borderColor=${s.borderColor}`);
  L.push(`showBorder=${b(s.showBorder)}`);
  L.push(`cornerRadius=${s.rounded ? s.cornerRadius : 0}`);
  L.push(`labelColor=${s.labelColor}`);
  L.push(`accentColor=${s.accentColor}`);
  L.push(`uppercase=${b(s.uppercase)}`);
  L.push(`letterSpacing=${s.letterSpacing}`);
  L.push(`titleStyle=${s.titleStyle}`);
  L.push(`padding=${s.padding}`);
  L.push(`rowGap=${s.rowGap}`);
  L.push(`blockGap=${s.blockGap}`);
  L.push(`separators=${b(s.showSeparators)}`);
  L.push(`shadow=${b(s.shadow)}`, '');

  L.push('[Hotkeys]');
  for (const a of HOTKEY_ACTIONS) {
    const h = s.hotkeys[a.id];
    L.push(`${a.id}=${vkCode(h.code)},${winMods(h)}   ; ${hotkeyLabel(h)} — ${a.label}`);
  }
  L.push('');

  s.blocks.forEach((blk, i) => {
    L.push(`[Block_${i}]`);
    L.push(`title=${blk.title}`);
    L.push(`showTitle=${b(blk.showTitle)}`);
    L.push(`enabled=${b(blk.enabled)}`);
    L.push(`hardware=${blk.hw}`);
    L.push(`itemCount=${blk.items.length}`, '');

    blk.items.forEach((it, j) => {
      const [r, g, bl] = hexToRgbf(it.color);
      L.push(`[Block_${i}.Item_${j}]`);
      L.push(`; ${SENSOR_MAP[it.sensor].name} (${SENSOR_MAP[it.sensor].unit || '—'})`);
      L.push(`sensor=${it.sensor}`);
      L.push(`label=${it.label}`);
      L.push(`showLabel=${b(it.showLabel)}`);
      L.push(`showUnit=${b(it.showUnit)}`);
      L.push(`sameLine=${b(it.sameLine)}`);
      L.push(`bigFont=${b(it.big)}`);
      L.push(`showGraph=${b(it.showGraph)}`);
      if (it.sensor === 'FPS') L.push(`frameGenBreakdown=${b(!!it.fgBreakdown)}`);
      L.push(`warningThreshold=${it.warning}`);
      L.push(`criticalThreshold=${it.critical}`);
      L.push(`colorR=${r.toFixed(3)}`);
      L.push(`colorG=${g.toFixed(3)}`);
      L.push(`colorB=${bl.toFixed(3)}`, '');
    });
  });

  return L.join('\n');
}

export function downloadIni(s: OverlaySettings, brand?: string) {
  const blob = new Blob([buildIni(s, brand)], { type: 'text/plain;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'config.ini';
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}
