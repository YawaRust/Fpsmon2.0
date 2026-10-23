import { useState } from 'react';
import { Check, Copy, Download, FileCode2, Folder, Terminal } from 'lucide-react';
import { Reveal, SectionHead, Chip } from './ui';
import type { BrandOption } from './Naming';

interface Node {
  depth: number;
  name: string;
  note?: string;
  folder?: boolean;
}

const TREE: Node[] = [
  { depth: 0, name: 'yawaside-overlay/', folder: true },
  { depth: 1, name: 'native/', folder: true, note: 'нативное приложение C++20' },
  { depth: 2, name: 'CMakeLists.txt', note: '/O2 · /GL · /LTCG · APP_VERSION из CI' },
  { depth: 2, name: 'src/', folder: true },
  { depth: 3, name: 'main.cpp', note: 'WinMain · трей · RegisterHotKey · loop' },
  { depth: 3, name: 'config/Model.cpp', note: 'блоки · темы · INI-парсер' },
  { depth: 3, name: 'core/Sensors.cpp', note: 'PDH · DXGI 1.4 · DWM кадры' },
  { depth: 3, name: 'overlay/Overlay.cpp', note: 'layered окно · D2D рендер блоков' },
  { depth: 3, name: 'overlay/ConfigUI.cpp', note: 'редактор ВНУТРИ оверлея · immediate-UI' },
  { depth: 2, name: 'resources/', folder: true },
  { depth: 3, name: 'resource.rc', note: 'version info из APP_VERSION' },
  { depth: 3, name: 'app.manifest', note: 'PerMonitorV2 DPI' },
  { depth: 1, name: '.github/workflows/', folder: true },
  { depth: 2, name: 'release.yml', note: 'версия · тег · EXE · changelog' },
  { depth: 2, name: 'ci.yml', note: 'сборка PR · commit-lint' },
  { depth: 1, name: 'CHANGELOG.md', note: 'обновляется автоматически' },
  { depth: 1, name: 'README.md' },
  { depth: 1, name: 'LICENSE', note: 'MIT' },
];

const COMMANDS_BASE = [
  'git clone https://github.com/__REPO__.git && cd __SLUG__',
  'cmake -B build -S native -G Ninja -DCMAKE_BUILD_TYPE=Release',
  'cmake --build build --parallel',
];

const REQUIREMENTS = [
  'Windows 10/11 x64',
  'MSVC 2022 · C++20',
  'CMake 3.20+',
  'Windows SDK 10.0+',
  'GPU с D3D11',
];

export default function Source({ brand }: { brand: BrandOption }) {
  const [copied, setCopied] = useState(false);

  const COMMANDS = COMMANDS_BASE.map((c) =>
    c.replace('__REPO__', brand.repo).replace('__SLUG__', brand.slug),
  );

  const copyAll = async () => {
    const text = COMMANDS.join('\n');
    try {
      await navigator.clipboard.writeText(text);
    } catch {
      const ta = document.createElement('textarea');
      ta.value = text;
      document.body.appendChild(ta);
      ta.select();
      document.execCommand('copy');
      ta.remove();
    }
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <section id="source" className="scroll-mt-20 py-24 lg:py-32">
      <div className="mx-auto max-w-7xl px-5 lg:px-8">
        <SectionHead
          index="06"
          tag="СБОРКА ИЗ ИСХОДНИКОВ"
          title={
            <>
              Открытый C++20.
              <br />
              <span className="text-neon">Собери за минуту.</span>
            </>
          }
          desc={
            <>
              Полный исходный код под Windows: WinAPI-окно настроек, трей-меню, INI-конфиг,
              горячие клавиши. Один CMakeLists — никаких зависимостей, кроме системных библиотек.
            </>
          }
        />

        <div className="grid gap-8 lg:grid-cols-2">
          {/* файловое дерево */}
          <Reveal>
            <div className="h-full overflow-hidden rounded-2xl border border-white/10 bg-panel">
              <div className="flex items-center gap-2.5 border-b border-white/8 px-5 py-3.5">
                <Folder size={14} className="text-neon" />
                <span className="font-mono text-[11px] tracking-[0.2em] text-slate-400">СТРУКТУРА РЕПОЗИТОРИЯ</span>
                <span className="ml-auto font-mono text-[10px] text-slate-600">v0.0.1 · 6 файлов .cpp</span>
              </div>
              <div className="p-4 font-mono text-[12px] leading-[1.9]">
                {TREE.map((n, i) => (
                  <div
                    key={i}
                    className="group flex items-baseline gap-2 rounded px-2 transition-colors hover:bg-white/[0.03]"
                    style={{ paddingLeft: `${n.depth * 20 + 8}px` }}
                  >
                    {n.folder ? (
                      <Folder size={12} className="translate-y-[1px] shrink-0 text-cyber/70" />
                    ) : (
                      <FileCode2 size={12} className="translate-y-[1px] shrink-0 text-slate-600" />
                    )}
                    <span className={n.folder ? 'text-cyber' : 'text-slate-300'}>{n.name}</span>
                    {n.note && (
                      <span className="ml-2 truncate text-[10px] text-slate-600 transition-colors group-hover:text-slate-500">
                        {'// '}{n.note}
                      </span>
                    )}
                  </div>
                ))}
              </div>
            </div>
          </Reveal>

          {/* терминал */}
          <Reveal delay={130}>
            <div className="flex h-full flex-col gap-5">
              <div className="overflow-hidden rounded-2xl border border-white/10 bg-[#070a0f]">
                <div className="flex items-center gap-2.5 border-b border-white/8 px-5 py-3.5">
                  <Terminal size={14} className="text-neon" />
                  <span className="font-mono text-[11px] tracking-[0.2em] text-slate-400">DEVELOPER COMMAND PROMPT</span>
                  <button
                    onClick={copyAll}
                    className={`ml-auto flex items-center gap-1.5 rounded-md border px-2.5 py-1.5 font-mono text-[10px] transition-all ${
                      copied
                        ? 'border-neon/50 bg-neon/10 text-neon'
                        : 'border-white/12 text-slate-400 hover:border-white/30 hover:text-white'
                    }`}
                  >
                    {copied ? <Check size={12} /> : <Copy size={12} />}
                    {copied ? 'скопировано' : 'копировать'}
                  </button>
                </div>
                <div className="p-5 font-mono text-[12.5px] leading-[2.15]">
                  {COMMANDS.map((c, i) => (
                    <div key={c} className="flex gap-3">
                      <span className="shrink-0 text-neon/70 select-none">{i === 0 ? 'λ' : '›'}</span>
                      <span className="break-all text-slate-300">{c}</span>
                    </div>
                  ))}
                  <div className="mt-2 flex gap-3">
                    <span className="text-neon/70 select-none">›</span>
                    <span className="text-slate-500">
                      YawaFPSHUD.exe <span className="text-neon">· 318 КБ</span> — один процесс, настройки внутри
                      <span className="animate-caret text-neon">_</span>
                    </span>
                  </div>
                </div>
              </div>

              {/* флаги MSVC */}
              <div className="rounded-2xl border border-white/10 bg-panel p-5">
                <div className="mb-3 font-mono text-[10px] tracking-[0.25em] text-slate-500">
                  ОПТИМИЗАЦИЯ В CMAKE LISTS
                </div>
                <div className="flex flex-wrap gap-2">
                  {['/O2', '/GL', '/LTCG', '/OPT:REF', '/OPT:ICF', '/GS-', '/Oi', '/Ot'].map((f) => (
                    <span key={f} className="rounded-md border border-neon/25 bg-neon/[0.06] px-2.5 py-1 font-mono text-[11px] text-neon">
                      {f}
                    </span>
                  ))}
                </div>
              </div>

              {/* требования + кнопка */}
              <div className="flex flex-wrap gap-2">
                {REQUIREMENTS.map((r) => (
                  <Chip key={r}>{r}</Chip>
                ))}
              </div>

              <button className="group flex items-center justify-between rounded-2xl border border-white/12 bg-gradient-to-r from-white/[0.04] to-transparent p-5 text-left transition-all hover:border-neon/40 hover:from-neon/[0.08]">
                <div>
                  <div className="text-[14px] font-bold text-white transition-colors group-hover:text-neon">
                    Скачать готовый Release
                  </div>
                  <div className="mt-1 font-mono text-[10.5px] text-slate-500">
                    {brand.slug}-v0.0.1-x64.zip · 318 КБ · portable, без установщика
                  </div>
                </div>
                <span className="grid h-11 w-11 shrink-0 place-items-center rounded-xl bg-neon text-ink transition-transform group-hover:scale-110">
                  <Download size={18} />
                </span>
              </button>
            </div>
          </Reveal>
        </div>
      </div>
    </section>
  );
}
