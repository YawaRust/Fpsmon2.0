import { useState } from 'react';
import {
  CheckCircle2, FileText, GitBranch, GitCommitHorizontal, Package, Rocket, Tag, Workflow,
} from 'lucide-react';
import { Reveal, SectionHead, Chip } from './ui';
import type { BrandOption } from './Naming';

/* ---------- шаги пайплайна ---------- */

const STEPS = [
  { icon: GitCommitHorizontal, title: 'push в main', desc: 'Conventional Commits: feat / fix / perf', time: '0 с' },
  { icon: Workflow, title: 'вычисление версии', desc: 'feat → minor, fix → patch, ! → major', time: '4 с' },
  { icon: Package, title: 'сборка на windows-latest', desc: 'MSVC + Ninja, Release, проверка размера', time: '52 с' },
  { icon: FileText, title: 'генерация changelog', desc: 'группировка коммитов по типам', time: '2 с' },
  { icon: Tag, title: 'авто-тег vX.Y.Z', desc: 'git tag -a + push origin', time: '1 с' },
  { icon: Rocket, title: 'GitHub Release', desc: 'EXE + ZIP + SHA256SUMS', time: '6 с' },
];

/* ---------- фрагменты YAML ---------- */

const YAML_VERSION = `- name: Вычислить новую версию
  id: bump
  run: |
    LAST=$(git describe --tags --abbrev=0 || echo "v0.0.0")
    IFS='.' read -r MAJOR MINOR PATCH <<< "\${LAST#v}"
    COMMITS=$(git log $LAST..HEAD --pretty=format:"%s")

    BUMP="patch"
    echo "$COMMITS" | grep -qE '^feat(\\(.+\\))?:' && BUMP="minor"
    echo "$COMMITS" | grep -qE 'BREAKING CHANGE' && BUMP="major"

    case "$BUMP" in
      major) MAJOR=$((MAJOR+1)); MINOR=0; PATCH=0 ;;
      minor) MINOR=$((MINOR+1)); PATCH=0 ;;
      patch) PATCH=$((PATCH+1)) ;;
    esac
    echo "new_version=$MAJOR.$MINOR.$PATCH" >> $GITHUB_OUTPUT`;

const YAML_BUILD = `build:
  needs: version
  runs-on: windows-latest
  steps:
    - uses: actions/checkout@v4
    - uses: ilammy/msvc-dev-cmd@v1
      with: { arch: x64 }

    - run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    - run: cmake --build build --parallel

    - name: Проверить бюджет размера
      run: |
        $kb = (Get-Item build/YawaFPSHUD.exe).Length / 1KB
        if ($kb -gt 1024) { Write-Error "превышен 1 МБ"; exit 1 }`;

const YAML_RELEASE = `- name: Опубликовать Release
  uses: softprops/action-gh-release@v2
  with:
    tag_name: v\${{ needs.version.outputs.new_version }}
    body: \${{ needs.version.outputs.changelog }}
    files: |
      assets/*-x64.zip
      assets/SHA256SUMS.txt
      assets/build/YawaFPSHUD.exe`;

const TABS = [
  { id: 'version', label: 'версия + тег', code: YAML_VERSION },
  { id: 'build', label: 'сборка EXE', code: YAML_BUILD },
  { id: 'release', label: 'публикация', code: YAML_RELEASE },
];

/* ---------- changelog-превью ---------- */

const CHANGELOG = [
  { type: '✨ Новое', color: '#00ff88', items: ['feat(themes): восемь готовых стилей оверлея', 'feat(framegen): разбор нативных и сгенерированных кадров', 'feat(hotkeys): свои сочетания клавиш'] },
  { type: '🐛 Исправления', color: '#ffd23f', items: ['fix(capture): WDA_NONE не применялся без рестарта'] },
  { type: '⚡ Производительность', color: '#00d8ff', items: ['perf(core): ноль аллокаций в главном цикле'] },
];

export default function Release({ brand }: { brand: BrandOption }) {
  const [tab, setTab] = useState('version');
  const code = TABS.find((t) => t.id === tab)!.code;

  return (
    <section id="release" className="relative scroll-mt-20 py-24 lg:py-28">
      <div className="mx-auto max-w-7xl px-5 lg:px-8">
        <SectionHead
          index="04"
          tag="GITHUB ACTIONS · АВТОРЕЛИЗ"
          title={<>Пушишь коммит —<br /><span className="text-neon">получаешь готовый EXE.</span></>}
          desc={
            <>
              Репозиторий <span className="font-mono text-[13px] text-slate-300">github.com/{brand.repo}</span> собирается
              сам: версия считается по Conventional Commits, тег ставится автоматически,
              changelog собирается из истории, а релиз с бинарником публикуется без единого клика.
            </>
          }
        />

        {/* конвейер */}
        <Reveal>
          <div className="mb-8 grid gap-2.5 sm:grid-cols-2 lg:grid-cols-6">
            {STEPS.map((s, i) => {
              const Icon = s.icon;
              return (
                <div key={s.title} className="group relative rounded-xl border border-white/8 bg-panel p-3.5 transition-colors hover:border-neon/30">
                  <div className="mb-2.5 flex items-center justify-between">
                    <Icon size={15} className="text-neon" />
                    <span className="font-mono text-[8.5px] text-slate-600">{String(i + 1).padStart(2, '0')}</span>
                  </div>
                  <div className="font-mono text-[10.5px] font-bold text-white">{s.title}</div>
                  <div className="mt-1 text-[10px] leading-snug text-slate-500">{s.desc}</div>
                  <div className="mt-2 font-mono text-[9px] text-neon/70">{s.time}</div>
                  {i < STEPS.length - 1 && (
                    <span className="absolute top-1/2 -right-[7px] z-10 hidden h-px w-3 bg-neon/30 lg:block" />
                  )}
                </div>
              );
            })}
          </div>
        </Reveal>

        <div className="grid gap-5 lg:grid-cols-[1.25fr_1fr]">
          {/* YAML */}
          <Reveal className="min-w-0">
            <div className="overflow-hidden rounded-2xl border border-white/10 bg-[#070a0f]">
              <div className="flex items-center gap-2 border-b border-white/8 px-4 py-3">
                <GitBranch size={13} className="text-neon" />
                <span className="font-mono text-[10.5px] tracking-[0.18em] text-slate-400">
                  .github/workflows/release.yml
                </span>
              </div>
              <div className="flex gap-1 border-b border-white/8 px-3 py-2">
                {TABS.map((t) => (
                  <button key={t.id} onClick={() => setTab(t.id)}
                    className={`rounded px-2.5 py-1 font-mono text-[9.5px] transition-colors ${
                      tab === t.id ? 'bg-neon/12 text-neon' : 'text-slate-500 hover:text-slate-200'
                    }`}>
                    {t.label}
                  </button>
                ))}
              </div>
              <pre className="overflow-x-auto p-4 font-mono text-[11px] leading-[1.75] text-slate-300">
                <code>
                  {code.split('\n').map((line, i) => (
                    <div key={i} className="flex">
                      <span className="w-7 shrink-0 select-none text-right text-slate-700">{i + 1}</span>
                      <span className="pl-3" style={{
                        color: line.trimStart().startsWith('#') ? '#4b5563'
                          : /^\s*-?\s*(name|uses|run|with|needs|id|runs-on|steps):/.test(line) ? '#7ee8fa'
                          : line.includes('$GITHUB_OUTPUT') || line.includes('${{') ? '#00ff88'
                          : undefined,
                      }}>
                        {line || ' '}
                      </span>
                    </div>
                  ))}
                </code>
              </pre>
            </div>
          </Reveal>

          {/* changelog + релиз */}
          <Reveal delay={120} className="min-w-0">
            <div className="space-y-4">
              <div className="overflow-hidden rounded-2xl border border-white/10 bg-panel">
                <div className="flex items-center gap-2 border-b border-white/8 px-4 py-3">
                  <FileText size={13} className="text-neon" />
                  <span className="font-mono text-[10.5px] tracking-[0.18em] text-slate-400">CHANGELOG.md</span>
                  <span className="ml-auto rounded bg-neon/12 px-1.5 py-0.5 font-mono text-[8.5px] text-neon">авто</span>
                </div>
                <div className="p-4">
                  {CHANGELOG.map((g) => (
                    <div key={g.type} className="mb-3 last:mb-0">
                      <div className="mb-1.5 font-mono text-[10px] font-bold" style={{ color: g.color }}>{g.type}</div>
                      {g.items.map((it) => (
                        <div key={it} className="flex gap-2 py-[2px] font-mono text-[9.5px] text-slate-400">
                          <span className="text-slate-700">·</span>
                          <span className="min-w-0 flex-1">{it}</span>
                        </div>
                      ))}
                    </div>
                  ))}
                </div>
              </div>

              {/* карточка релиза */}
              <div className="rounded-2xl border border-neon/25 bg-neon/[0.04] p-4">
                <div className="flex items-center gap-2">
                  <Tag size={14} className="text-neon" />
                  <span className="font-display text-[14px] font-bold text-white">{brand.name} v0.0.1</span>
                  <span className="ml-auto flex items-center gap-1 rounded-full bg-neon px-2 py-0.5 font-mono text-[8.5px] font-bold text-ink">
                    <CheckCircle2 size={9} /> LATEST
                  </span>
                </div>
                <div className="mt-3 space-y-1.5">
                  {[
                    `${brand.slug}-v0.0.1-x64.zip`,
                    'YawaFPSHUD.exe',
                    'SHA256SUMS.txt',
                  ].map((f) => (
                    <div key={f} className="flex items-center gap-2 rounded border border-white/8 bg-ink/50 px-2.5 py-1.5">
                      <Package size={11} className="shrink-0 text-slate-500" />
                      <span className="min-w-0 flex-1 truncate font-mono text-[9.5px] text-slate-300">{f}</span>
                      <span className="shrink-0 font-mono text-[8.5px] text-slate-600">318 КБ</span>
                    </div>
                  ))}
                </div>
                <div className="mt-3 flex flex-wrap gap-1.5">
                  <Chip>semantic-release</Chip>
                  <Chip>MSVC x64</Chip>
                  <Chip>без установщика</Chip>
                </div>
              </div>
            </div>
          </Reveal>
        </div>
      </div>
    </section>
  );
}
