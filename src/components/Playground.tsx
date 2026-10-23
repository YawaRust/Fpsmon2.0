import { useCallback, useEffect, useRef, useState, type CSSProperties, type PointerEvent as RPE } from 'react';
import { Cpu, Lock, Move, Radio, Settings2, SlidersHorizontal, Sparkles, Video, VideoOff } from 'lucide-react';
import Overlay from './Overlay';
import ConfigPanel from './ConfigPanel';
import { Reveal, SectionHead } from './ui';
import {
  defaultSettings, hotkeyLabel, hotkeyMatches, makeBlock, makeItem,
  type HotkeyAction, type OverlayPos, type OverlaySettings,
} from '../lib/types';
import { THEMES, applyTheme } from '../lib/themes';
import type { FrameGenMode, SimEngine, SimSnapshot } from '../lib/sim';
import gameScene from '../assets/game-scene.jpg';

/* ---------------- пресеты раскладки ---------------- */

const PRESETS: { id: string; name: string; build: (s: OverlaySettings) => OverlaySettings }[] = [
  {
    id: 'full', name: 'Полный',
    build: (s) => ({ ...defaultSettings(), theme: s.theme, fontSize: s.fontSize, position: s.position, customX: s.customX, customY: s.customY, hotkeys: s.hotkeys, captureMode: s.captureMode }),
  },
  {
    id: 'compact', name: 'Компактный',
    build: (s) => ({
      ...s, direction: 'column', blockGap: 4,
      blocks: [
        makeBlock('fps', '', [makeItem('FPS', { label: '', showLabel: false, big: true, showGraph: true, fgBreakdown: true })], { showTitle: false }),
        makeBlock('cpu', 'CPU', [makeItem('CPUUsage', { label: '', showLabel: false }), makeItem('CPUTemp', { label: '', showLabel: false, sameLine: true })]),
        makeBlock('gpu', 'GPU', [makeItem('GPUUsage', { label: '', showLabel: false }), makeItem('GPUTemp', { label: '', showLabel: false, sameLine: true })]),
        makeBlock('ram', 'RAM', [makeItem('RAMUsage', { label: '', showLabel: false })]),
      ],
    }),
  },
  {
    id: 'bar', name: 'Полоска',
    build: (s) => ({
      ...s, direction: 'row', blockGap: 10, position: 'tl',
      blocks: [
        makeBlock('fps', '', [makeItem('FPS', { label: 'FPS', fgBreakdown: true })], { showTitle: false }),
        makeBlock('cpu', 'CPU', [makeItem('CPUUsage', { label: '', showLabel: false }), makeItem('CPUTemp', { label: '', showLabel: false, sameLine: true })]),
        makeBlock('gpu', 'GPU', [makeItem('GPUUsage', { label: '', showLabel: false }), makeItem('GPUTemp', { label: '', showLabel: false, sameLine: true })]),
        makeBlock('ram', 'RAM', [makeItem('RAMUsage', { label: '', showLabel: false })]),
      ],
    }),
  },
  {
    id: 'fg', name: 'Frame Gen',
    build: (s) => ({
      ...s, direction: 'column',
      blocks: [
        makeBlock('fps', 'FRAMES', [
          makeItem('FPS', { label: '', showLabel: false, big: true, showGraph: true, fgBreakdown: true }),
          makeItem('FPSNative', { label: 'NAT', showGraph: true }),
          makeItem('FPSGen', { label: 'GEN', sameLine: true }),
          makeItem('FrameGenX', { label: 'MODE' }),
          makeItem('Latency', { label: 'LAT', sameLine: true }),
          makeItem('FPS1Low', { label: '1%' }),
          makeItem('Frametime', { label: 'FT', sameLine: true }),
        ]),
        makeBlock('gpu', 'GPU', [
          makeItem('GPUUsage', { label: '', showLabel: false, showGraph: true }),
          makeItem('GPUTemp', { label: '', showLabel: false, sameLine: true }),
          makeItem('VRAM', { label: 'VRAM' }),
          makeItem('GPUPower', { label: 'PWR', sameLine: true }),
        ]),
      ],
    }),
  },
  {
    id: 'bench', name: 'Бенчмарк',
    build: (s) => ({
      ...s, direction: 'column',
      blocks: [
        makeBlock('fps', 'FRAMES', [
          makeItem('FPS', { label: '', showLabel: false, big: true, showGraph: true, fgBreakdown: true }),
          makeItem('FPSAvg', { label: 'AVG', sameLine: true }),
          makeItem('FPS1Low', { label: '1%', sameLine: true }),
          makeItem('Frametime', { label: 'FT', showGraph: true }),
          makeItem('Latency', { label: 'LAT', sameLine: true }),
        ]),
        makeBlock('cpu', 'CPU', [
          makeItem('CPUUsage', { label: '', showLabel: false, showGraph: true }),
          makeItem('CPUTemp', { label: '', showLabel: false, sameLine: true }),
          makeItem('CPUPower', { label: 'PWR' }), makeItem('CPUClock', { label: 'CLK', sameLine: true }),
        ]),
        makeBlock('gpu', 'GPU', [
          makeItem('GPUUsage', { label: '', showLabel: false, showGraph: true }),
          makeItem('GPUTemp', { label: '', showLabel: false, sameLine: true }),
          makeItem('GPUPower', { label: 'PWR' }), makeItem('GPUClock', { label: 'CLK', sameLine: true }),
          makeItem('VRAM', { label: 'VRAM' }), makeItem('GPUFan', { label: 'FAN', sameLine: true }),
        ]),
      ],
    }),
  },
];

const FG_MODES: { v: FrameGenMode; label: string; hint: string }[] = [
  { v: 0, label: 'OFF', hint: 'генерация кадров выключена' },
  { v: 2, label: '2×', hint: 'DLSS 3 / FSR 3 — один сгенерированный кадр' },
  { v: 3, label: '3×', hint: 'DLSS 4 MFG — два сгенерированных кадра' },
  { v: 4, label: '4×', hint: 'DLSS 4 MFG — три сгенерированных кадра' },
];

/* ---------------- drag ---------------- */

function useDrag(frameRef: React.RefObject<HTMLDivElement | null>, onMove: (x: number, y: number) => void) {
  const state = useRef<{ dx: number; dy: number; el: HTMLElement } | null>(null);

  const start = useCallback((e: RPE<HTMLDivElement>, handleOnly?: boolean) => {
    const el = e.currentTarget as HTMLElement;
    if (handleOnly && !(e.target as HTMLElement).closest('[data-drag-handle]')) return;
    const r = el.getBoundingClientRect();
    state.current = { dx: e.clientX - r.left, dy: e.clientY - r.top, el };
    el.setPointerCapture(e.pointerId);
  }, []);

  const move = useCallback((e: RPE<HTMLDivElement>) => {
    const st = state.current;
    const frame = frameRef.current;
    if (!st || !frame) return;
    const fr = frame.getBoundingClientRect();
    const er = st.el.getBoundingClientRect();
    const x = Math.min(Math.max(e.clientX - fr.left - st.dx, 0), Math.max(0, fr.width - er.width));
    const y = Math.min(Math.max(e.clientY - fr.top - st.dy, 0), Math.max(0, fr.height - er.height));
    onMove(Math.round(x), Math.round(y));
  }, [frameRef, onMove]);

  const end = useCallback(() => { state.current = null; }, []);
  return { start, move, end };
}

/* ---------------- песочница ---------------- */

export default function Playground({
  snap, engine, brand, isEmbedded = false,
}: { snap: SimSnapshot; engine: SimEngine; brand: string; isEmbedded?: boolean }) {
  const [settings, setSettings] = useState<OverlaySettings>(() => defaultSettings());

  // Синхронизация с нативным хостом (WebView2)
  useEffect(() => {
    if (!isEmbedded || !window.chrome?.webview) return;

    const onMessage = (event: any) => {
      if (event.data && typeof event.data === 'object') {
        // C++ прислал актуальные настройки
        setSettings(event.data);
      }
    };

    window.chrome?.webview?.addEventListener('message', onMessage);
    // Сообщаем C++, что мы готовы принимать данные
    window.chrome?.webview?.postMessage({ type: 'ready' });

    return () => window.chrome?.webview?.removeEventListener('message', onMessage);
  }, [isEmbedded]);

  // При любом изменении настроек в React — отправляем их в C++
  useEffect(() => {
    if (!isEmbedded || !window.chrome?.webview) return;
    window.chrome?.webview?.postMessage({ type: 'update', settings });
  }, [settings, isEmbedded]);
  const [configOpen, setConfigOpen] = useState(true);
  const [overlayOn, setOverlayOn] = useState(true);
  const [panelPos, setPanelPos] = useState({ x: 0, y: 0 });
  const [frameSize, setFrameSize] = useState({ w: 0, h: 560 });
  const [selBlock, setSelBlock] = useState<string | null>(null);
  const [selItem, setSelItem] = useState<string | null>(null);
  const [preset, setPreset] = useState('full');
  const [fg, setFg] = useState<FrameGenMode>(2);
  const [toast, setToast] = useState<string | null>(null);

  const frameRef = useRef<HTMLDivElement | null>(null);
  const placed = useRef(false);

  useEffect(() => { engine.setIntervals(settings.updateInterval, settings.fpsInterval); },
    [engine, settings.updateInterval, settings.fpsInterval]);
  useEffect(() => { engine.setFrameGen(fg); }, [engine, fg]);

  useEffect(() => {
    const f = frameRef.current;
    if (!f) return;
    const apply = () => {
      const r = f.getBoundingClientRect();
      setFrameSize({ w: r.width, h: r.height });
      if (!placed.current) {
        placed.current = true;
        setPanelPos({ x: Math.max(12, r.width - 338 - 16), y: 16 });
      }
    };
    apply();
    const ro = new ResizeObserver(apply);
    ro.observe(f);
    return () => ro.disconnect();
  }, []);

  const flash = (msg: string) => {
    setToast(msg);
    setTimeout(() => setToast((t) => (t === msg ? null : t)), 1600);
  };

  /* --- рабочие горячие клавиши (как RegisterHotKey) --- */
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const t = e.target as HTMLElement | null;
      if (t && (t.tagName === 'INPUT' || t.tagName === 'TEXTAREA' || t.isContentEditable)) return;

      if (e.code === 'Escape') { setConfigOpen(false); return; }

      const hk = settings.hotkeys;
      const fire = (a: HotkeyAction) => hotkeyMatches(hk[a], e);

      if (fire('toggleOverlay')) {
        e.preventDefault(); setOverlayOn((v) => !v); flash('оверлей переключён');
      } else if (fire('toggleConfig')) {
        e.preventDefault(); setConfigOpen((v) => !v);
        document.getElementById('playground')?.scrollIntoView({ behavior: 'smooth', block: 'center' });
      } else if (fire('cycleTheme')) {
        e.preventDefault();
        const i = THEMES.findIndex((t) => t.id === settings.theme);
        const next = THEMES[(i + 1) % THEMES.length];
        setSettings((s) => applyTheme(s, next.id));
        flash(`тема: ${next.name}`);
      } else if (fire('cyclePosition')) {
        e.preventDefault();
        const order: OverlayPos[] = ['tl', 'tr', 'br', 'bl'];
        const i = order.indexOf(settings.position);
        const next = order[(i + 1) % order.length];
        setSettings((s) => ({ ...s, position: next }));
        flash(`позиция: ${next.toUpperCase()}`);
      } else if (fire('resetStats')) {
        e.preventDefault(); engine.resetStats(); flash('AVG / 1% LOW сброшены');
      } else if (fire('toggleCapture')) {
        e.preventDefault();
        setSettings((s) => ({ ...s, captureMode: s.captureMode === 'allow' ? 'exclude' : 'allow' }));
        flash('режим захвата переключён');
      }
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [settings.hotkeys, settings.theme, settings.position, engine]);

  const overlayDrag = useDrag(frameRef, (x, y) => setSettings((s) => ({ ...s, position: 'custom', customX: x, customY: y })));
  const panelDrag = useDrag(frameRef, (x, y) => setPanelPos({ x, y }));

  const posStyle: CSSProperties =
    settings.position === 'custom' ? { left: settings.customX, top: settings.customY }
      : settings.position === 'tr' ? { right: 14, top: 14 }
      : settings.position === 'bl' ? { left: 14, bottom: 14 }
      : settings.position === 'br' ? { right: 14, bottom: 14 }
      : { left: 14, top: 14 };

  const cpuCost = {
    ram: (4.2 + (snap.values.CPUUsage % 4) * 0.55 + (configOpen ? 1.3 : 0)).toFixed(1),
    cpu: (0.11 + (snap.values.GPUUsage % 8) * 0.035 + (configOpen ? 0.14 : 0)).toFixed(2),
  };

  const captured = settings.captureMode === 'allow';

  if (isEmbedded) {
    return (
      <div className="fixed inset-0 flex items-center justify-center p-4">
         <ConfigPanel
            settings={settings} setSettings={setSettings} brand={brand}
            onClose={() => window.chrome?.webview?.postMessage({ type: 'close' })}
            onSelectBlock={setSelBlock} onSelectItem={setSelItem}
            cpuCost={cpuCost} maxHeight={window.innerHeight - 100}
          />
      </div>
    );
  }

  return (
    <section id="playground" className="relative scroll-mt-20 py-24 lg:py-28">
      <div className="mx-auto max-w-[1400px] px-5 lg:px-8">
        <SectionHead
          index="01"
          tag="НАСТРОЙКА ПРЯМО В ИГРЕ"
          title={<>Весь интерфейс — это оверлей.<br /><span className="text-neon">Сворачивать игру не нужно.</span></>}
          desc={
            <>
              {hotkeyLabel(settings.hotkeys.toggleConfig)} открывает редактор поверх игры: блоки по оборудованию,
              темы, свои хоткеи, автозапуск и режим захвата для OBS. Всё работает по-настоящему —
              попробуйте нажать сочетания прямо сейчас.
            </>
          }
        />

        {/* тулбар */}
        <Reveal>
          <div className="mb-3 flex flex-wrap items-center gap-2">
            {PRESETS.map((p) => (
              <button key={p.id} onClick={() => { setPreset(p.id); setSettings((s) => p.build(s)); }}
                className={`rounded-md border px-3 py-1.5 font-mono text-[10.5px] transition-colors ${
                  preset === p.id ? 'border-neon/60 bg-neon/10 text-neon' : 'border-white/12 text-slate-400 hover:border-white/30 hover:text-white'
                }`}>
                {p.name}
              </button>
            ))}

            {/* фреймген игры */}
            <div className="ml-auto flex items-center gap-1.5 rounded-md border border-fuchsia-400/25 bg-fuchsia-400/[0.06] px-2 py-1">
              <Sparkles size={12} className="text-fuchsia-300" />
              <span className="font-mono text-[9.5px] tracking-wider text-fuchsia-300">FRAME GEN В ИГРЕ</span>
              {FG_MODES.map((m) => (
                <button key={m.v} onClick={() => setFg(m.v)} title={m.hint}
                  className={`rounded px-1.5 py-[3px] font-mono text-[9.5px] transition-colors ${
                    fg === m.v ? 'bg-fuchsia-400 text-ink' : 'text-slate-400 hover:text-fuchsia-200'
                  }`}>
                  {m.label}
                </button>
              ))}
            </div>

            <button onClick={() => setConfigOpen((v) => !v)}
              className={`flex items-center gap-2 rounded-md border px-3 py-1.5 font-mono text-[10.5px] transition-colors ${
                configOpen ? 'border-neon/60 bg-neon/10 text-neon' : 'border-white/12 text-slate-300 hover:border-neon/40 hover:text-neon'
              }`}>
              <Settings2 size={12} />
              {hotkeyLabel(settings.hotkeys.toggleConfig)}
            </button>
          </div>
        </Reveal>

        <div className="grid gap-4 xl:grid-cols-[1fr_280px]">
          {/* игровой кадр */}
          <Reveal delay={80} className="min-w-0">
            <div ref={frameRef} className="relative aspect-[16/9] min-h-[430px] touch-none overflow-hidden rounded-2xl border border-white/12 select-none">
              <img src={gameScene} alt="Игровая сцена" className="pointer-events-none absolute inset-0 h-full w-full object-cover" />
              <div className="pointer-events-none absolute inset-0 bg-gradient-to-t from-black/45 via-transparent to-black/25" />
              <div className="scanlines pointer-events-none absolute inset-0 opacity-50" />

              <div className="pointer-events-none absolute bottom-4 left-5 font-mono text-[10px] tracking-[0.25em] text-white/30">
                CYBERRUSH // DX12 · {fg >= 2 ? `FRAME GEN ${fg}×` : 'NATIVE'}
              </div>

              {/* оверлей */}
              {overlayOn && (
                <div
                  onPointerDown={(e) => !settings.clickThrough && overlayDrag.start(e)}
                  onPointerMove={overlayDrag.move}
                  onPointerUp={overlayDrag.end}
                  className={`absolute ${settings.clickThrough ? 'pointer-events-none' : 'cursor-move'}`}
                  style={posStyle}
                >
                  <Overlay snap={snap} settings={settings} selectedBlock={selBlock} selectedItem={selItem} />
                </div>
              )}

              {/* редактор */}
              {configOpen && (
                <div
                  onPointerDown={(e) => panelDrag.start(e, true)}
                  onPointerMove={panelDrag.move}
                  onPointerUp={panelDrag.end}
                  className="absolute"
                  style={{ left: panelPos.x, top: panelPos.y }}
                >
                  <ConfigPanel
                    settings={settings} setSettings={setSettings} brand={brand}
                    onClose={() => setConfigOpen(false)}
                    onSelectBlock={setSelBlock} onSelectItem={setSelItem}
                    cpuCost={cpuCost} maxHeight={frameSize.h - panelPos.y - 16}
                  />
                </div>
              )}

              {settings.clickThrough && (
                <div className="pointer-events-none absolute top-4 left-1/2 flex -translate-x-1/2 items-center gap-2 rounded-lg border border-cyber/30 bg-ink/85 px-3 py-2 backdrop-blur-sm">
                  <Lock size={12} className="text-cyber" />
                  <span className="font-mono text-[10px] text-cyber">WS_EX_TRANSPARENT — клики уходят в игру</span>
                </div>
              )}

              {!configOpen && (
                <button onClick={() => setConfigOpen(true)}
                  className="absolute top-4 right-4 flex items-center gap-2 rounded-lg border border-white/15 bg-ink/80 px-3 py-2 font-mono text-[10px] text-slate-300 backdrop-blur-sm transition-colors hover:border-neon/50 hover:text-neon">
                  <SlidersHorizontal size={12} /> {hotkeyLabel(settings.hotkeys.toggleConfig)}
                </button>
              )}

              {toast && (
                <div className="pointer-events-none absolute bottom-5 left-1/2 -translate-x-1/2 rounded-lg border border-neon/40 bg-ink/90 px-4 py-2 font-mono text-[10.5px] text-neon backdrop-blur-sm">
                  {toast}
                </div>
              )}
            </div>
          </Reveal>

          {/* правая колонка: OBS + фреймген */}
          <Reveal delay={150} className="min-w-0">
            <div className="space-y-3">
              {/* OBS превью */}
              <div className="overflow-hidden rounded-xl border border-white/10 bg-panel">
                <div className="flex items-center gap-2 border-b border-white/8 px-3 py-2">
                  <Radio size={12} className={captured ? 'text-danger' : 'text-slate-600'} />
                  <span className="font-mono text-[9.5px] tracking-[0.18em] text-slate-400">OBS · ЗАПИСЬ ЭКРАНА</span>
                  <span className={`ml-auto h-1.5 w-1.5 rounded-full ${captured ? 'animate-blink bg-danger' : 'bg-slate-700'}`} />
                </div>
                <div className="relative aspect-video">
                  <img src={gameScene} alt="Превью записи OBS" className="absolute inset-0 h-full w-full object-cover" />
                  <div className="absolute inset-0 bg-black/25" />
                  {/* оверлей виден только если захват разрешён */}
                  {captured && overlayOn && (
                    <div className="absolute top-1.5 left-1.5 origin-top-left scale-[0.42]">
                      <Overlay snap={snap} settings={settings} />
                    </div>
                  )}
                  <div className="absolute right-1.5 bottom-1.5 rounded bg-black/70 px-1.5 py-0.5 font-mono text-[7.5px] text-white/70">
                    1920×1080 · 60 fps
                  </div>
                </div>
                <div className="p-2.5">
                  <button
                    onClick={() => setSettings((s) => ({ ...s, captureMode: s.captureMode === 'allow' ? 'exclude' : 'allow' }))}
                    className={`flex w-full items-center justify-center gap-2 rounded-md border py-2 font-mono text-[10px] transition-colors ${
                      captured ? 'border-neon/50 bg-neon/10 text-neon' : 'border-white/12 text-slate-400 hover:text-white'
                    }`}>
                    {captured ? <Video size={12} /> : <VideoOff size={12} />}
                    захват сторонних оверлеев: {captured ? 'вкл' : 'выкл'}
                  </button>
                  <p className="mt-2 font-mono text-[8.5px] leading-relaxed text-slate-600">
                    {captured
                      ? 'WDA_NONE — оверлей попадает в OBS, Discord и скриншоты. Для бенчмарк-видео и стримов.'
                      : 'WDA_EXCLUDEFROMCAPTURE — зрители видят чистую игру, оверлей только на вашем мониторе.'}
                  </p>
                </div>
              </div>

              {/* разбор фреймгена */}
              <div className="rounded-xl border border-fuchsia-400/20 bg-fuchsia-400/[0.04] p-3">
                <div className="mb-2 flex items-center gap-1.5">
                  <Cpu size={12} className="text-fuchsia-300" />
                  <span className="font-mono text-[9.5px] tracking-[0.18em] text-fuchsia-300">РАЗБОР КАДРОВ</span>
                </div>
                {[
                  ['итог на экране', Math.round(snap.values.FPS), '#22ff66'],
                  ['нативные', Math.round(snap.values.FPSNative), '#7fe0ff'],
                  ['сгенерированные', Math.round(snap.values.FPSGen), '#ff9dff'],
                  ['задержка, мс', Math.round(snap.values.Latency), '#ffd23f'],
                ].map(([k, v, c]) => (
                  <div key={k as string} className="mb-1.5 last:mb-0">
                    <div className="flex justify-between font-mono text-[9.5px]">
                      <span className="text-slate-400">{k}</span>
                      <span className="tabular-nums" style={{ color: c as string }}>{v as number}</span>
                    </div>
                    <div className="mt-1 h-[3px] overflow-hidden rounded-full bg-white/8">
                      <div className="h-full rounded-full transition-all duration-500"
                        style={{ width: `${Math.min(100, (v as number) / 2.4)}%`, background: c as string }} />
                    </div>
                  </div>
                ))}
                <p className="mt-2 font-mono text-[8px] leading-relaxed text-slate-500">
                  present-хук считает реальные и интерполированные кадры раздельно: видно честный
                  нативный FPS и цену генерации по задержке.
                </p>
              </div>
            </div>
          </Reveal>
        </div>

        {/* легенда хоткеев */}
        <Reveal delay={200}>
          <div className="mt-4 flex flex-wrap items-center gap-x-5 gap-y-2 rounded-xl border border-white/8 bg-panel/60 px-4 py-3">
            <span className="flex items-center gap-1.5 font-mono text-[9.5px] tracking-[0.2em] text-slate-500">
              <Move size={11} /> ХОТКЕИ АКТИВНЫ:
            </span>
            {[
              ['toggleOverlay', 'оверлей'], ['toggleConfig', 'редактор'], ['cycleTheme', 'тема'],
              ['cyclePosition', 'угол'], ['resetStats', 'сброс AVG'], ['toggleCapture', 'захват OBS'],
            ].map(([a, label]) => (
              <span key={a} className="flex items-center gap-1.5">
                <kbd className="rounded border border-white/15 bg-white/[0.05] px-1.5 py-0.5 font-mono text-[9px] text-slate-200">
                  {hotkeyLabel(settings.hotkeys[a as HotkeyAction])}
                </kbd>
                <span className="font-mono text-[9px] text-slate-500">{label}</span>
              </span>
            ))}
          </div>
        </Reveal>
      </div>
    </section>
  );
}
