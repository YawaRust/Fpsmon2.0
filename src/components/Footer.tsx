import { ArrowUp, Download, Keyboard } from 'lucide-react';
import { Reveal, GitHubIcon } from './ui';
import type { BrandOption } from './Naming';

export default function Footer({ brand }: { brand: BrandOption }) {
  return (
    <footer className="relative overflow-hidden border-t border-white/8">
      {/* CTA */}
      <div className="relative py-24 lg:py-32">
        <div className="pointer-events-none absolute inset-0">
          <div className="absolute bottom-0 left-1/2 h-[380px] w-[820px] -translate-x-1/2 rounded-full bg-neon/[0.07] blur-[110px]" />
        </div>
        <div className="relative mx-auto max-w-7xl px-5 text-center lg:px-8">
          <Reveal>
            <div className="mb-6 inline-flex items-center gap-2 font-mono text-[10.5px] tracking-[0.3em] text-slate-500">
              <span className="h-px w-8 bg-white/20" />
              v0.0.1 · X64 · 318 КБ
              <span className="h-px w-8 bg-white/20" />
            </div>
          </Reveal>
          <Reveal delay={90}>
            <h2 className="font-display text-[clamp(2rem,6vw,4.6rem)] leading-[1.02] font-black text-white">
              Поставь поверх игры —<br />
              <span className="text-stroke-neon">и забудь, что он запущен.</span>
            </h2>
          </Reveal>
          <Reveal delay={180}>
            <div className="mt-10 flex flex-wrap items-center justify-center gap-4">
              <a
                href="#source"
                className="inline-flex items-center gap-2.5 rounded-lg bg-neon px-8 py-4 text-[14.5px] font-bold text-ink transition-all hover:-translate-y-0.5 hover:shadow-[0_14px_50px_rgba(0,255,136,0.4)]"
              >
                <Download size={17} />
                Скачать для Windows
              </a>
              <a
                href="#source"
                className="inline-flex items-center gap-2.5 rounded-lg border border-white/15 px-8 py-4 text-[14.5px] font-semibold text-slate-200 transition-colors hover:border-neon/50 hover:text-neon"
              >
                <GitHubIcon size={17} />
                Открыть репозиторий
              </a>
            </div>
          </Reveal>
        </div>
      </div>

      {/* нижняя панель */}
      <div className="border-t border-white/8 bg-panel/60">
        <div className="mx-auto flex max-w-7xl flex-wrap items-center gap-x-8 gap-y-4 px-5 py-6 lg:px-8">
          <div className="font-mono text-[11px] text-slate-500">
            <span className="text-white">{brand.name}</span>
            <span className="ml-3 text-slate-600">// github.com/{brand.repo} · MIT</span>
          </div>

          <div className="flex items-center gap-2 font-mono text-[10.5px] text-slate-500">
            <Keyboard size={13} className="text-slate-600" />
            <kbd className="rounded border border-white/12 bg-white/[0.04] px-1.5 py-0.5">Shift+F12</kbd>
            оверлей
            <kbd className="ml-2 rounded border border-white/12 bg-white/[0.04] px-1.5 py-0.5">Shift+F11</kbd>
            редактор в игре
          </div>

          <div className="ml-auto flex items-center gap-5">
            <span className="font-mono text-[10px] text-slate-600">© 2026 · build 0.0.1 · early access</span>
            <a
              href="#top"
              className="grid h-9 w-9 place-items-center rounded-full border border-white/12 text-slate-400 transition-all hover:border-neon/60 hover:text-neon"
              aria-label="Наверх"
            >
              <ArrowUp size={15} />
            </a>
          </div>
        </div>
      </div>
    </footer>
  );
}
