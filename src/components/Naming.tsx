import { Check, Globe, Package, Tag } from 'lucide-react';
import { Reveal, SectionHead } from './ui';

export interface BrandOption {
  id: string;
  name: string;
  /** как пишется в коде / exe */
  slug: string;
  repo: string;
  tagline: string;
  desc: string;
  mark: string;
  accent: string;
  /** стиль логотипа */
  logo: 'split' | 'mono' | 'bracket' | 'dot' | 'slash';
}

export const BRANDS: BrandOption[] = [
  {
    id: 'yawafps-hud',
    name: 'YawaFPS HUD',
    slug: 'yawafps-hud',
    repo: 'yawaside/yawafps-hud',
    tagline: 'Выбор автора · официальное имя',
    desc: 'Ник Yawa + FPS + HUD: продукт читается сразу — «худ от yawaside». Ставим по умолчанию для релизов, тегов и changelog.',
    mark: 'YF',
    accent: '#00ff88',
    logo: 'split',
  },
  {
    id: 'yawahud',
    name: 'YawaHUD',
    slug: 'yawahud',
    repo: 'yawaside/yawahud',
    tagline: 'Короткое и продуктовое',
    desc: 'Сжатие ника до «Yawa» + HUD. Легко произносится, отлично ложится на иконку и имя процесса YawaHUD.exe.',
    mark: 'YH',
    accent: '#00d8ff',
    logo: 'mono',
  },
  {
    id: 'aside',
    name: 'ASIDE',
    slug: 'aside',
    repo: 'yawaside/aside',
    tagline: 'Вторая половина ника',
    desc: '«Aside» — то, что сбоку экрана. Игра слов с ником и прямое указание на оверлей у края.',
    mark: 'AS',
    accent: '#ff2bd1',
    logo: 'bracket',
  },
  {
    id: 'yawaside-pulse',
    name: 'yawaside Pulse',
    slug: 'pulse',
    repo: 'yawaside/pulse',
    tagline: 'Про живые метрики',
    desc: 'Пульс системы: графики, частоты, температуры. Хорошо масштабируется в линейку (Pulse, Pulse Bench).',
    mark: 'yP',
    accent: '#ff9955',
    logo: 'dot',
  },
  {
    id: 'ywsd',
    name: 'YWSD Monitor',
    slug: 'ywsd',
    repo: 'yawaside/ywsd',
    tagline: 'Техничный консонантный код',
    desc: 'Аббревиатура ника в духе RTSS и HWiNFO. Выглядит как утилита системного уровня.',
    mark: 'YW',
    accent: '#b9a7ff',
    logo: 'mono',
  },
  {
    id: 'sidebar',
    name: 'yawaside/FRAME',
    slug: 'frame',
    repo: 'yawaside/frame',
    tagline: 'Акцент на кадрах и фреймгене',
    desc: 'Подчёркивает главную фичу: честный разбор нативных и сгенерированных кадров.',
    mark: 'yF',
    accent: '#ffd23f',
    logo: 'slash',
  },
];

function Logo({ b, size = 'md' }: { b: BrandOption; size?: 'md' | 'lg' }) {
  const box = size === 'lg' ? 'h-12 w-12 text-[15px]' : 'h-10 w-10 text-[13px]';
  const common = `grid ${box} shrink-0 place-items-center font-display font-black`;

  if (b.logo === 'split')
    return (
      <span className={`${common} overflow-hidden rounded-xl`} style={{ background: b.accent, color: '#05070b' }}>
        {b.mark}
      </span>
    );
  if (b.logo === 'bracket')
    return (
      <span className={`${common} rounded-md border-2`} style={{ borderColor: b.accent, color: b.accent }}>
        {b.mark}
      </span>
    );
  if (b.logo === 'dot')
    return (
      <span className={`${common} relative rounded-full border`} style={{ borderColor: `${b.accent}66`, color: b.accent }}>
        {b.mark}
        <i className="absolute top-1 right-1 h-1.5 w-1.5 rounded-full" style={{ background: b.accent }} />
      </span>
    );
  if (b.logo === 'slash')
    return (
      <span className={`${common} skew-x-[-10deg] rounded-sm`} style={{ background: `${b.accent}1f`, color: b.accent, borderLeft: `3px solid ${b.accent}` }}>
        {b.mark}
      </span>
    );
  return (
    <span className={`${common} rounded-lg bg-white/[0.06]`} style={{ color: b.accent }}>
      {b.mark}
    </span>
  );
}

export default function Naming({
  active, onPick,
}: { active: string; onPick: (b: BrandOption) => void }) {
  return (
    <section id="naming" className="relative scroll-mt-20 border-y border-white/6 bg-panel/40 py-24 lg:py-28">
      <div className="mx-auto max-w-7xl px-5 lg:px-8">
        <SectionHead
          index="02"
          tag="НЕЙМИНГ ПОД БРЕНД YAWASIDE"
          title={<>Шесть вариантов названия.<br /><span className="text-neon">Выберите — применится везде.</span></>}
          desc={
            <>
              Имя подставляется в шапку сайта, заголовок встроенного редактора, имя процесса,
              ключ автозапуска в реестре и путь GitHub-репозитория. Кликните по карточке,
              чтобы увидеть результат вживую.
            </>
          }
        />

        <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-3">
          {BRANDS.map((b, i) => {
            const on = active === b.id;
            return (
              <Reveal key={b.id} delay={(i % 3) * 90}>
                <button
                  onClick={() => onPick(b)}
                  className={`group relative h-full w-full overflow-hidden rounded-2xl border p-5 text-left transition-all duration-300 ${
                    on ? 'border-neon/55 bg-neon/[0.05]' : 'border-white/8 bg-ink/60 hover:-translate-y-1 hover:border-white/25'
                  }`}
                >
                  <div
                    className="pointer-events-none absolute -top-16 -right-16 h-36 w-36 rounded-full blur-3xl transition-opacity duration-300"
                    style={{ background: `${b.accent}22`, opacity: on ? 1 : 0 }}
                  />

                  <div className="flex items-start gap-3">
                    <Logo b={b} size="lg" />
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center gap-2">
                        <span className="font-display text-[16px] font-bold text-white">{b.name}</span>
                        {on && (
                          <span className="flex items-center gap-1 rounded-full bg-neon px-2 py-0.5 font-mono text-[8.5px] font-bold text-ink">
                            <Check size={9} strokeWidth={3} /> ВЫБРАНО
                          </span>
                        )}
                      </div>
                      <div className="mt-0.5 font-mono text-[10px] tracking-wide" style={{ color: b.accent }}>
                        {b.tagline}
                      </div>
                    </div>
                  </div>

                  <p className="mt-3.5 text-[12.5px] leading-relaxed text-slate-400">{b.desc}</p>

                  <div className="mt-4 space-y-1.5 border-t border-white/8 pt-3.5">
                    {[
                      { icon: Package, text: `${b.slug}-v0.0.1-x64.zip` },
                      { icon: Globe, text: `github.com/${b.repo}` },
                      { icon: Tag, text: `HKCU\\…\\Run\\${b.name.replace(/[\s/]/g, '')}` },
                    ].map((r) => {
                      const Icon = r.icon;
                      return (
                        <div key={r.text} className="flex items-center gap-2 font-mono text-[9.5px] text-slate-500">
                          <Icon size={10} className="shrink-0 text-slate-600" />
                          <span className="truncate">{r.text}</span>
                        </div>
                      );
                    })}
                  </div>
                </button>
              </Reveal>
            );
          })}
        </div>
      </div>
    </section>
  );
}
