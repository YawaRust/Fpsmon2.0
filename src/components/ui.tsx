import { useEffect, useState, type ReactNode } from 'react';
import { useInView } from '../hooks/useInView';

/* ---------------- Reveal ---------------- */

export function Reveal({
  children,
  delay = 0,
  className = '',
}: {
  children: ReactNode;
  delay?: number;
  className?: string;
}) {
  const { ref, inView } = useInView<HTMLDivElement>(0.14);
  return (
    <div
      ref={ref}
      className={`${className} transition-all duration-[900ms] ease-[cubic-bezier(0.22,1,0.36,1)] ${
        inView ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-7'
      }`}
      style={{ transitionDelay: `${delay}ms` }}
    >
      {children}
    </div>
  );
}

/* ---------------- SectionHead ---------------- */

export function SectionHead({
  index,
  tag,
  title,
  desc,
}: {
  index: string;
  tag: string;
  title: ReactNode;
  desc?: ReactNode;
}) {
  return (
    <div className="mb-12 lg:mb-16">
      <Reveal>
        <div className="mb-5 flex items-center gap-3 font-mono text-[11px] tracking-[0.32em] text-neon">
          <span className="h-px w-8 bg-neon/50" />
          {index} // {tag}
        </div>
      </Reveal>
      <Reveal delay={90}>
        <h2 className="font-display text-[clamp(1.75rem,4.2vw,3.2rem)] leading-[1.08] font-bold text-white">
          {title}
        </h2>
      </Reveal>
      {desc ? (
        <Reveal delay={180}>
          <p className="mt-5 max-w-2xl text-[15px] leading-relaxed text-dim">{desc}</p>
        </Reveal>
      ) : null}
    </div>
  );
}

/* ---------------- Counter ---------------- */

export function Counter({
  to,
  decimals = 0,
  prefix = '',
  suffix = '',
  duration = 1500,
  className = '',
}: {
  to: number;
  decimals?: number;
  prefix?: string;
  suffix?: string;
  duration?: number;
  className?: string;
}) {
  const { ref, inView } = useInView<HTMLSpanElement>(0.5);
  const [v, setV] = useState(0);

  useEffect(() => {
    if (!inView) return;
    let raf = 0;
    const t0 = performance.now();
    const step = (t: number) => {
      const p = Math.min(1, (t - t0) / duration);
      const e = 1 - Math.pow(1 - p, 3);
      setV(to * e);
      if (p < 1) raf = requestAnimationFrame(step);
    };
    raf = requestAnimationFrame(step);
    return () => cancelAnimationFrame(raf);
  }, [inView, to, duration]);

  return (
    <span ref={ref} className={className}>
      {prefix}
      {v.toFixed(decimals)}
      {suffix}
    </span>
  );
}

/* ---------------- Toggle ---------------- */

export function Toggle({
  checked,
  onChange,
  label,
  desc,
}: {
  checked: boolean;
  onChange: (v: boolean) => void;
  label: string;
  desc?: string;
}) {
  return (
    <button
      type="button"
      onClick={() => onChange(!checked)}
      className="group flex w-full items-center justify-between gap-4 py-2 text-left"
    >
      <span className="min-w-0">
        <span className="block text-[13.5px] font-medium text-slate-200 group-hover:text-white">
          {label}
        </span>
        {desc ? <span className="mt-0.5 block text-[11.5px] leading-snug text-slate-500">{desc}</span> : null}
      </span>
      <span
        className={`relative mt-0.5 h-[20px] w-[36px] shrink-0 rounded-full transition-colors duration-200 ${
          checked ? 'bg-neon' : 'bg-white/12 group-hover:bg-white/20'
        }`}
      >
        <span
          className={`absolute top-[2.5px] h-[15px] w-[15px] rounded-full transition-all duration-200 ${
            checked ? 'left-[18.5px] bg-ink' : 'left-[2.5px] bg-slate-400'
          }`}
        />
      </span>
    </button>
  );
}

/* ---------------- SliderRow ---------------- */

export function SliderRow({
  label,
  value,
  min,
  max,
  step = 1,
  onChange,
  format,
}: {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  onChange: (v: number) => void;
  format?: (v: number) => string;
}) {
  return (
    <label className="block py-2">
      <span className="mb-2.5 flex items-baseline justify-between">
        <span className="text-[13.5px] font-medium text-slate-200">{label}</span>
        <span className="font-mono text-[11.5px] text-neon">{format ? format(value) : value}</span>
      </span>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        className="slider w-full"
      />
    </label>
  );
}

/* ---------------- GitHub icon (brand, нет в lucide) ---------------- */

export function GitHubIcon({ size = 16, className = '' }: { size?: number; className?: string }) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
      className={className}
      aria-hidden
    >
      <path d="M15 22v-4a4.8 4.8 0 0 0-1-3.5c3 0 6-2 6-5.5.08-1.25-.27-2.48-1-3.5.28-1.15.28-2.35 0-3.5 0 0-1 0-3 1.5-2.64-.5-5.36-.5-8 0C6 2 5 2 5 2c-.3 1.15-.3 2.35 0 3.5A5.403 5.403 0 0 0 4 9c0 3.5 3 5.5 6 5.5-.39.49-.68 1.05-.85 1.65-.17.6-.22 1.23-.15 1.85v4" />
      <path d="M9 18c-4.51 2-5-2-7-2" />
    </svg>
  );
}

/* ---------------- Chip ---------------- */

export function Chip({ children, className = '' }: { children: ReactNode; className?: string }) {
  return (
    <span
      className={`inline-flex items-center gap-1.5 rounded-full border border-white/10 bg-white/[0.04] px-3 py-1 font-mono text-[10.5px] tracking-wide text-slate-300 ${className}`}
    >
      {children}
    </span>
  );
}
