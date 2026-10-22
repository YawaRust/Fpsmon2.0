import { useEffect, useState } from 'react';
import { Activity } from 'lucide-react';
import { GitHubIcon } from './ui';
import type { BrandOption } from './Naming';

const LINKS = [
  { href: '#playground', label: 'Демо' },
  { href: '#naming', label: 'Нейминг' },
  { href: '#performance', label: 'Нагрузка' },
  { href: '#release', label: 'Авторелиз' },
  { href: '#source', label: 'Сборка' },
];

export default function Nav({ fps, brand }: { fps: number; brand: BrandOption }) {
  const [scrolled, setScrolled] = useState(false);

  useEffect(() => {
    const onScroll = () => setScrolled(window.scrollY > 24);
    onScroll();
    window.addEventListener('scroll', onScroll, { passive: true });
    return () => window.removeEventListener('scroll', onScroll);
  }, []);

  return (
    <header
      className={`fixed inset-x-0 top-0 z-50 transition-all duration-300 ${
        scrolled ? 'border-b border-white/8 bg-ink/80 backdrop-blur-md' : 'bg-transparent'
      }`}
    >
      <div className="mx-auto flex h-16 max-w-7xl items-center gap-4 px-5 lg:px-8">
        <a href="#top" className="group flex items-center gap-2.5">
          <span
            className="grid h-8 w-8 place-items-center rounded-lg font-display text-[12px] font-black text-ink shadow-[0_0_18px_rgba(0,255,136,0.35)] transition-transform duration-300 group-hover:rotate-6"
            style={{ background: brand.accent }}
          >
            {brand.mark}
          </span>
          <span className="font-display text-[13px] font-bold tracking-[0.1em] whitespace-nowrap text-white">
            {brand.name}
          </span>
          <span className="hidden rounded border border-white/15 px-1.5 py-0.5 font-mono text-[9.5px] text-slate-400 sm:block">
            v0.0.1
          </span>
        </a>

        <nav className="ml-auto hidden items-center gap-7 md:flex">
          {LINKS.map((l) => (
            <a
              key={l.href}
              href={l.href}
              className="text-[13px] font-medium text-slate-400 transition-colors hover:text-neon"
            >
              {l.label}
            </a>
          ))}
        </nav>

        <div className="ml-auto flex items-center gap-2.5 md:ml-0">
          <div className="hidden items-center gap-2 rounded-full border border-neon/25 bg-neon/[0.06] px-3 py-1.5 sm:flex">
            <Activity size={12} className="text-neon" />
            <span className="font-mono text-[11px] font-bold text-neon tabular-nums">
              {Math.round(fps)} FPS
            </span>
            <span className="h-1.5 w-1.5 animate-blink rounded-full bg-neon" />
          </div>
          <a
            href="#source"
            className="grid h-9 w-9 place-items-center rounded-full border border-white/12 text-slate-300 transition-all hover:border-neon/60 hover:text-neon"
            aria-label={`Репозиторий ${brand.repo}`}
            title={`github.com/${brand.repo}`}
          >
            <GitHubIcon size={16} />
          </a>
        </div>
      </div>
    </header>
  );
}
