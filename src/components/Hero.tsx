import { ArrowDown, EyeOff, Keyboard, RefreshCw } from 'lucide-react';
import Overlay from './Overlay';
import { Counter, GitHubIcon } from './ui';
import gameScene from '../assets/game-scene.jpg';
import type { SimSnapshot } from '../lib/sim';
import { defaultSettings, makeBlock, makeItem, type OverlaySettings } from '../lib/types';
import type { BrandOption } from './Naming';

function heroSettings(): OverlaySettings {
  const s = defaultSettings();
  s.fontSize = 15;
  s.opacity = 0.96;
  s.padding = 12;
  s.blocks = [
    makeBlock('fps', '', [
      makeItem('FPS', { label: '', showLabel: false, big: true, showGraph: true, fgBreakdown: true }),
      makeItem('FPS1Low', { label: '1%', sameLine: true }),
      makeItem('Latency', { label: 'LAT', sameLine: true }),
    ], { showTitle: false }),
    makeBlock('cpu', 'CPU', [
      makeItem('CPUUsage', { label: '', showLabel: false, showGraph: true }),
      makeItem('CPUTemp', { label: '', showLabel: false, sameLine: true }),
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
  ];
  return s;
}

/** создаём один раз — uid элементов не должны меняться между рендерами */
const HERO_SETTINGS = heroSettings();

const STATS: { value: number; decimals?: number; prefix?: string; suffix: string; label: string }[] = [
  { value: 8, suffix: ' МБ', label: 'пиковое ОЗУ процесса' },
  { value: 0.5, decimals: 1, prefix: '<', suffix: ' %', label: 'нагрузка на CPU в фоне' },
  { value: 8, suffix: '', label: 'готовых тем оформления' },
  { value: 4, suffix: '×', label: 'режимы frame gen: 2× / 3× / 4×' },
];

export default function Hero({
  snap,
  brand,
  overlayVisible,
  onToggleOverlay,
}: {
  snap: SimSnapshot;
  brand: BrandOption;
  overlayVisible: boolean;
  onToggleOverlay: () => void;
}) {
  return (
    <section id="top" className="relative overflow-hidden pt-28 pb-16 lg:pt-40 lg:pb-24">
      {/* фон */}
      <div className="grid-bg absolute inset-0" />
      <div className="pointer-events-none absolute -top-40 left-1/2 h-[560px] w-[900px] -translate-x-1/2 rounded-full bg-neon/[0.07] blur-[120px]" />
      <div className="pointer-events-none absolute top-1/3 -right-40 h-[400px] w-[400px] rounded-full bg-cyber/[0.05] blur-[100px]" />

      <div className="relative mx-auto grid max-w-7xl items-center gap-14 px-5 lg:grid-cols-12 lg:gap-10 lg:px-8">
        {/* левая колонка */}
        <div className="lg:col-span-5">
          <div className="mb-6 inline-flex items-center gap-2 rounded-full border border-neon/30 bg-neon/[0.07] px-3.5 py-1.5 font-mono text-[10.5px] tracking-[0.18em] text-neon">
            <span className="h-1.5 w-1.5 animate-blink rounded-full bg-neon" />
            v0.0.1 · АВТОРЕЛИЗ НА GITHUB · FRAME GEN
          </div>

          <h1 className="font-display leading-[0.95] font-black text-white">
            <span className="block text-[clamp(2.4rem,6.4vw,4.6rem)]">{brand.name.split(/[\s/]/)[0]}</span>
            <span className="text-stroke-neon block text-[clamp(2.4rem,6.4vw,4.6rem)]">
              {brand.name.split(/[\s/]/).slice(1).join(' ') || 'OVERLAY'}
            </span>
          </h1>
          <div className="mt-2 font-mono text-[11px] tracking-[0.3em] text-slate-500">
            github.com/{brand.repo}<span className="animate-caret text-neon">_</span>
          </div>

          <p className="mt-7 max-w-md text-[15.5px] leading-relaxed text-dim">
            Блоки по оборудованию — CPU, GPU, память, диск и сеть — поверх любой игры.
            Переименовывайте показатели, двигайте строки, собирайте их в одну линию
            и настраивайте всё <span className="text-slate-200">прямо в оверлее</span>,
            не сворачивая игру.
          </p>

          <div className="mt-9 flex flex-wrap items-center gap-3.5">
            <a
              href="#playground"
              className="group inline-flex items-center gap-2.5 rounded-lg bg-neon px-6 py-3.5 text-[14px] font-bold text-ink transition-all duration-200 hover:-translate-y-0.5 hover:shadow-[0_12px_40px_rgba(0,255,136,0.35)]"
            >
              Живое демо настроек
              <ArrowDown size={16} className="transition-transform duration-200 group-hover:translate-y-0.5" />
            </a>
            <a
              href="#source"
              className="inline-flex items-center gap-2.5 rounded-lg border border-white/15 px-6 py-3.5 text-[14px] font-semibold text-slate-200 transition-colors hover:border-neon/50 hover:text-neon"
            >
              <GitHubIcon size={16} />
              Исходники C++
            </a>
          </div>

          <div className="mt-12 grid grid-cols-2 gap-x-8 gap-y-7">
            {STATS.map((s) => (
              <div key={s.label} className="border-l border-white/10 pl-4">
                <div className="font-display text-[clamp(1.5rem,2.6vw,2.1rem)] font-bold text-white">
                  <Counter
                    to={s.value}
                    decimals={s.decimals ?? 0}
                    prefix={s.prefix ?? ''}
                    suffix={s.suffix}
                  />
                </div>
                <div className="mt-1 text-[11.5px] leading-snug text-slate-500">{s.label}</div>
              </div>
            ))}
          </div>
        </div>

        {/* правая колонка — "окно игры" */}
        <div className="relative lg:col-span-7">
          <div className="pointer-events-none absolute -inset-8 rounded-[32px] bg-neon/[0.06] blur-3xl" />

          <div className="animate-floaty relative">
            <div className="relative overflow-hidden rounded-2xl border border-white/12 shadow-[0_40px_120px_rgba(0,0,0,0.6)]">
              {/* заголовок окна */}
              <div className="flex h-10 items-center gap-3 border-b border-white/8 bg-[#0a0c11] px-4">
                <span className="flex gap-1.5">
                  <i className="h-2.5 w-2.5 rounded-full bg-[#3a4150]" />
                  <i className="h-2.5 w-2.5 rounded-full bg-[#3a4150]" />
                  <i className="h-2.5 w-2.5 rounded-full bg-neon/70" />
                </span>
                <span className="font-mono text-[10.5px] tracking-wider text-slate-500">
                  CyberRush.exe — DirectX 12 — 2560×1440
                </span>
                <span className="ml-auto flex items-center gap-1.5 font-mono text-[10px] text-neon">
                  <span className="h-1.5 w-1.5 animate-blink rounded-full bg-neon" />
                  LIVE DEMO
                </span>
              </div>

              {/* сцена */}
              <div className="relative aspect-[16/10]">
                <img
                  src={gameScene}
                  alt="Демонстрация оверлея поверх игры"
                  className="absolute inset-0 h-full w-full object-cover"
                />
                <div className="absolute inset-0 bg-gradient-to-t from-black/45 via-transparent to-black/25" />
                <div className="scanlines absolute inset-0 opacity-60" />

                {/* оверлей */}
                {overlayVisible && (
                  <div className="absolute top-3 left-3">
                    <Overlay snap={snap} settings={HERO_SETTINGS} />
                  </div>
                )}

                {/* HUD-подпись */}
                <div className="absolute bottom-3 left-4 font-mono text-[10px] tracking-[0.25em] text-white/35">
                  ZONE-47 // SESSION 12:44
                </div>
                <div className="absolute right-4 bottom-3 font-mono text-[10px] tracking-wider text-white/35">
                  RTX ON · REFLEX
                </div>

                {/* углы */}
                <i className="absolute top-2 right-2 h-4 w-4 border-t border-r border-neon/50" />
                <i className="absolute bottom-2 left-2 h-4 w-4 border-b border-l border-neon/50" />
              </div>
            </div>

            {/* плавающие чипы */}
            <div className="absolute -bottom-5 left-6 hidden items-center gap-2 rounded-lg border border-white/10 bg-panel/90 px-3.5 py-2.5 backdrop-blur-sm md:flex">
              <Keyboard size={14} className="text-neon" />
              <button
                onClick={onToggleOverlay}
                className="font-mono text-[11px] text-slate-300 transition-colors hover:text-neon"
              >
                H — {overlayVisible ? 'скрыть' : 'показать'} оверлей
              </button>
            </div>

            <div className="absolute -top-4 -right-3 hidden rotate-2 items-center gap-2 rounded-lg border border-white/10 bg-panel/90 px-3.5 py-2 backdrop-blur-sm xl:flex">
              <EyeOff size={13} className="text-cyber" />
              <span className="font-mono text-[10px] text-slate-400">WDA_EXCLUDEFROMCAPTURE</span>
            </div>

            <div className="absolute -right-5 bottom-16 hidden -rotate-2 items-center gap-2 rounded-lg border border-white/10 bg-panel/90 px-3.5 py-2 backdrop-blur-sm xl:flex">
              <RefreshCw size={13} className="text-neon" />
              <span className="font-mono text-[10px] text-slate-400">ring-buffer × 120 samples</span>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
