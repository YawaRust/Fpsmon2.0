import { Cpu, MemoryStick, ShieldCheck } from 'lucide-react';
import { useInView } from '../hooks/useInView';
import { Reveal, SectionHead } from './ui';
import gpuDark from '../assets/gpu-dark.jpg';

interface Row {
  name: string;
  ram: number;  // МБ
  cpu: number;  // %
  stack: string;
  ours?: boolean;
}

const ROWS: Row[] = [
  { name: 'yawaside Overlay', ram: 5, cpu: 0.3, stack: 'C++ · WinAPI · Direct2D', ours: true },
  { name: 'RivaTuner RTSS', ram: 16, cpu: 0.9, stack: 'C/C++ · нативный хук' },
  { name: 'HWiNFO64', ram: 42, cpu: 1.4, stack: 'нативный · полный сенсорный пул' },
  { name: 'MSI Afterburner', ram: 55, cpu: 2.1, stack: 'нативный + плагины' },
  { name: 'FPS Monitor (офиц.)', ram: 92, cpu: 3.2, stack: 'QT-интерфейс' },
  { name: 'NZXT CAM', ram: 410, cpu: 6.8, stack: 'Electron · Chromium' },
];

function Bars({
  rows,
  metric,
  max,
  unit,
  inView,
}: {
  rows: Row[];
  metric: 'ram' | 'cpu';
  max: number;
  unit: string;
  inView: boolean;
}) {
  return (
    <div className="space-y-4">
      {rows.map((r, i) => {
        const v = r[metric];
        const pct = Math.max(2.5, (v / max) * 100);
        return (
          <div key={r.name}>
            <div className="mb-1.5 flex items-baseline justify-between gap-3">
              <span className={`truncate text-[12.5px] font-medium ${r.ours ? 'text-white' : 'text-slate-400'}`}>
                {r.name}
                {r.ours && (
                  <span className="ml-2 rounded bg-neon/15 px-1.5 py-0.5 font-mono text-[8.5px] tracking-[0.15em] text-neon">
                    ВЫ ЗДЕСЬ
                  </span>
                )}
              </span>
              <span
                className={`font-mono text-[13px] font-bold tabular-nums transition-opacity duration-500 ${
                  r.ours ? 'text-neon' : 'text-slate-300'
                } ${inView ? 'opacity-100' : 'opacity-0'}`}
                style={{ transitionDelay: `${i * 90 + 500}ms` }}
              >
                {metric === 'cpu' ? v.toFixed(1) : v} {unit}
              </span>
            </div>
            <div className="h-[9px] overflow-hidden rounded-full bg-white/[0.06]">
              <div
                className={`h-full rounded-full transition-[width] duration-[1100ms] ease-[cubic-bezier(0.22,1,0.36,1)] ${
                  r.ours
                    ? 'bg-gradient-to-r from-neon/80 to-neon shadow-[0_0_16px_rgba(0,255,136,0.5)]'
                    : 'bg-gradient-to-r from-slate-600/70 to-slate-500/70'
                }`}
                style={{
                  width: inView ? `${pct}%` : '0%',
                  transitionDelay: `${i * 90}ms`,
                }}
              />
            </div>
            <div className="mt-1 font-mono text-[9px] tracking-wide text-slate-600">{r.stack}</div>
          </div>
        );
      })}
    </div>
  );
}

const PRINCIPLES = [
  {
    icon: MemoryStick,
    title: 'Ring-буферы',
    text: 'История датчиков — фиксированные массивы на 120 сэмплов. Ни одной аллокации в главном цикле.',
    chip: '0 malloc / frame',
  },
  {
    icon: Cpu,
    title: 'MsgWaitForMultipleObjects',
    text: 'Поток спит до сообщения или таймера вместо busy-loop. Приоритет процесса — BELOW_NORMAL.',
    chip: 'CPU < 0.5 %',
  },
  {
    icon: ShieldCheck,
    title: 'WDA_EXCLUDEFROMCAPTURE',
    text: 'Оверлей невидим для стримов и скриншотов. SetWindowDisplayAffinity — один вызов при старте.',
    chip: 'приватность',
  },
];

export default function Performance() {
  const { ref, inView } = useInView<HTMLDivElement>(0.25);

  return (
    <section id="performance" className="relative scroll-mt-20 overflow-hidden border-y border-white/6 bg-panel/40 py-24 lg:py-32">
      {/* фон */}
      <div className="pointer-events-none absolute inset-0">
        <img src={gpuDark} alt="" className="absolute right-0 top-0 h-full w-1/2 object-cover opacity-[0.16]" />
        <div className="absolute inset-0 bg-gradient-to-r from-ink via-ink/85 to-ink/40" />
      </div>
      <div className="pointer-events-none absolute top-10 left-0 hidden font-display text-[19vw] leading-none font-black text-white/[0.025] select-none xl:block">
        0.5%
      </div>

      <div className="relative mx-auto max-w-7xl px-5 lg:px-8">
        <SectionHead
          index="03"
          tag="НАГРУЗКА НА СИСТЕМУ"
          title={
            <>
              Сверхмалое потребление.
              <br />
              <span className="text-stroke-neon">Замерено, а не нарисовано.</span>
            </>
          }
          desc={
            <>
              Сравнение резидентной памяти и средней загрузки CPU при работающем оверлее
              (idle-замер, Windows 11 23H2, обновление раз в секунду). Electron-мониторы
              тратят на один чекбокс больше, чем весь наш процесс целиком.
            </>
          }
        />

        <div ref={ref} className="grid gap-12 lg:grid-cols-2 lg:gap-16">
          <Reveal>
            <div className="mb-6 flex items-center gap-2.5 font-mono text-[11px] tracking-[0.25em] text-slate-400">
              <MemoryStick size={14} className="text-neon" />
              ОПЕРАТИВНАЯ ПАМЯТЬ, МБ
            </div>
            <Bars rows={ROWS} metric="ram" max={410} unit="МБ" inView={inView} />
          </Reveal>
          <Reveal delay={140}>
            <div className="mb-6 flex items-center gap-2.5 font-mono text-[11px] tracking-[0.25em] text-slate-400">
              <Cpu size={14} className="text-cyber" />
              СРЕДНЯЯ ЗАГРУЗКА CPU, %
            </div>
            <Bars rows={ROWS} metric="cpu" max={6.8} unit="%" inView={inView} />
          </Reveal>
        </div>

        <div className="mt-16 grid gap-4 md:grid-cols-3">
          {PRINCIPLES.map((p, i) => {
            const Icon = p.icon;
            return (
              <Reveal key={p.title} delay={i * 110}>
                <div className="group relative h-full overflow-hidden rounded-2xl border border-white/8 bg-ink/70 p-6 transition-colors hover:border-neon/30">
                  <div className="absolute -top-10 -right-10 h-28 w-28 rounded-full bg-neon/[0.06] blur-2xl transition-opacity opacity-0 group-hover:opacity-100" />
                  <Icon size={20} className="text-neon" />
                  <div className="mt-4 font-mono text-[13px] font-bold text-white">{p.title}</div>
                  <p className="mt-2 text-[12.5px] leading-relaxed text-slate-400">{p.text}</p>
                  <div className="mt-4 inline-block rounded border border-neon/25 bg-neon/[0.07] px-2 py-0.5 font-mono text-[9.5px] tracking-[0.15em] text-neon">
                    {p.chip}
                  </div>
                </div>
              </Reveal>
            );
          })}
        </div>

        <Reveal delay={200}>
          <p className="mt-8 font-mono text-[9.5px] leading-relaxed text-slate-600">
            * типовые значения для демонстрации порядка величин: замер через Process Explorer,
            после 60 с работы оверлея, без открытых окон настроек.
          </p>
        </Reveal>
      </div>
    </section>
  );
}
