import { useEffect, useRef, useState } from 'react';
import Nav from './components/Nav';
import Hero from './components/Hero';
import Playground from './components/Playground';
import Naming, { BRANDS, type BrandOption } from './components/Naming';
import Performance from './components/Performance';
import Release from './components/Release';
import Architecture from './components/Architecture';
import Source from './components/Source';
import Footer from './components/Footer';
import { SimEngine } from './lib/sim';
import { useSim } from './hooks/useSim';

/* ---------------- boot ---------------- */

const BOOT_LINES = [
  '> yawafps hud 0.0.1 · x64 ......... start',
  '> pdh + dxgi sensors .............. OK',
  '> present hook · frame gen ........ detected',
  '> layout: cpu / gpu / ram / io .... parsed',
  '> in-overlay config ............... ready',
];

function BootScreen({ fade, brand }: { fade: boolean; brand: string }) {
  return (
    <div className={`fixed inset-0 z-[200] flex items-center justify-center bg-ink transition-opacity duration-500 ${fade ? 'opacity-0' : 'opacity-100'}`}>
      <div className="w-[min(440px,86vw)]">
        <div className="mb-6 flex items-center gap-3">
          <span className="grid h-9 w-9 place-items-center rounded-lg bg-neon font-mono text-[13px] font-black text-ink">YO</span>
          <div>
            <div className="font-display text-[13px] font-bold tracking-[0.15em] text-white">
              {brand.toUpperCase()}
            </div>
            <div className="font-mono text-[9.5px] tracking-[0.2em] text-slate-600">INITIALISING OVERLAY</div>
          </div>
        </div>
        <div className="mb-6 min-h-[110px] font-mono text-[11px] leading-[1.9] text-slate-400">
          {BOOT_LINES.map((l, i) => (
            <div key={l} className="boot-line" style={{ animationDelay: `${i * 160}ms` }}>{l}</div>
          ))}
        </div>
        <div className="h-[3px] overflow-hidden rounded-full bg-white/8">
          <div className="boot-bar h-full rounded-full bg-neon shadow-[0_0_14px_rgba(0,255,136,0.7)]" />
        </div>
      </div>
    </div>
  );
}

/* ---------------- marquee ---------------- */

const MARQUEE_ITEMS = [
  'БЛОКИ ПО ЖЕЛЕЗУ', 'FRAME GEN 2×/3×/4×', 'СВОИ ХОТКЕИ', '8 ТЕМ', 'ЗАХВАТ В OBS',
  'АВТОЗАПУСК', 'АВТОРЕЛИЗ НА GITHUB', 'НАСТРОЙКА В ИГРЕ', '1 ПРОЦЕСС',
];

function Marquee() {
  return (
    <div className="overflow-hidden border-y border-white/8 bg-neon py-3.5">
      <div className="marquee-track gap-8">
        {[0, 1].map((dup) => (
          <div key={dup} className="flex shrink-0 items-center gap-8">
            {MARQUEE_ITEMS.map((item) => (
              <span key={item} className="flex items-center gap-8 whitespace-nowrap">
                <span className="font-display text-[14px] font-bold tracking-wide text-ink">{item}</span>
                <i className="h-1.5 w-1.5 rotate-45 bg-ink/50" />
              </span>
            ))}
          </div>
        ))}
      </div>
    </div>
  );
}

/* ---------------- app ---------------- */

declare global {
  interface Window {
    chrome?: {
      webview?: {
        postMessage: (message: any) => void;
        addEventListener: (type: string, listener: (event: any) => void) => void;
        removeEventListener: (type: string, listener: (event: any) => void) => void;
      };
    };
  }
}

export default function App() {
  const engineRef = useRef<SimEngine | null>(null);
  if (!engineRef.current) engineRef.current = new SimEngine();
  const engine = engineRef.current;
  const snap = useSim(engine);

  const [brand, setBrand] = useState<BrandOption>(BRANDS[0]);
  const [heroOverlay, setHeroOverlay] = useState(true);
  const [bootPhase, setBootPhase] = useState<'boot' | 'fade' | 'done'>('boot');
  
  // Режим "Встроенного конфига" для WebView2
  const isNativeHost = !!window.chrome?.webview;

  useEffect(() => {
    const t1 = setTimeout(() => setBootPhase('fade'), 1450);
    const t2 = setTimeout(() => setBootPhase('done'), 2000);
    return () => { clearTimeout(t1); clearTimeout(t2); };
  }, []);

  if (isNativeHost) {
    // В нативном режиме мы рендерим ТОЛЬКО Playground (который содержит ConfigPanel)
    // Весь остальной сайт (Hero, Footer и т.д.) не нужен внутри EXE
    return (
      <div className="bg-transparent overflow-hidden">
         <Playground snap={snap} engine={engine} brand={brand.name} isEmbedded={true} />
      </div>
    );
  }

  return (
    <div className="noise relative min-h-screen bg-ink text-slate-100">
      {bootPhase !== 'done' && <BootScreen fade={bootPhase === 'fade'} brand={brand.name} />}
      <Nav fps={snap.values.FPS} brand={brand} />
      <main>
        <Hero snap={snap} brand={brand} overlayVisible={heroOverlay} onToggleOverlay={() => setHeroOverlay((v) => !v)} />
        <Marquee />
        <Playground snap={snap} engine={engine} brand={brand.name} />
        <Naming active={brand.id} onPick={setBrand} />
        <Performance />
        <Release brand={brand} />
        <Architecture />
        <Source brand={brand} />
        <Footer brand={brand} />
      </main>
    </div>
  );
}
