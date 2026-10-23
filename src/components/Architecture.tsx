import {
  Code2, Database, EyeOff, Gauge, Infinity as InfinityIcon, Layers, Sparkles, Timer, Video, Zap,
} from 'lucide-react';
import { Reveal, SectionHead } from './ui';

const CARDS = [
  {
    icon: Code2,
    title: 'Один процесс, одно окно',
    text: 'Редактор настроек рисуется тем же Direct2D-контекстом поверх игры. Нет второго GUI-процесса, окна Comctl32 и трея-посредника.',
    tag: 'настройки в игре',
  },
  {
    icon: Timer,
    title: 'Адаптивный sleep',
    text: 'MsgWaitForMultipleObjects вместо busy-loop: поток просыпается ровно на опрос датчиков и перерисовку.',
    tag: 'CPU < 0.5 %',
  },
  {
    icon: Gauge,
    title: 'Direct2D + DWrite',
    text: 'Отрисовка оверлея и редактора аппаратная: ClearType-текст и примитивы считает GPU, а не CPU.',
    tag: 'GPU < 1 %',
  },
  {
    icon: Sparkles,
    title: 'Разбор Frame Gen',
    text: 'Present-хук считает реальные и интерполированные кадры раздельно: видно честный нативный FPS, множитель и цену по задержке.',
    tag: 'DLSS 3/4 · FSR 3',
  },
  {
    icon: Video,
    title: 'Режим захвата',
    text: 'Переключатель WDA_NONE / WDA_EXCLUDEFROMCAPTURE: оверлей либо попадает в OBS и скриншоты, либо остаётся только на вашем мониторе.',
    tag: 'стрим-френдли',
  },
  {
    icon: Database,
    title: 'PDH-счётчики',
    text: 'Диск и сеть — через Performance Data Helper. Никаких тяжёлых WMI-запросов с COM-маршалингом каждую секунду.',
    tag: 'диск / сеть',
  },
  {
    icon: Layers,
    title: 'Блоки по железу',
    text: 'Layout — массив Block с элементами Item: порядок, sameLine, своё имя и пороги. Перестройка раскладки не трогает опрос датчиков.',
    tag: 'CPU · GPU · RAM · I/O',
  },
  {
    icon: EyeOff,
    title: 'Автозапуск',
    text: 'Реестр HKCU\\Run без прав администратора либо планировщик задач с задержкой и запуском до входа в систему.',
    tag: 'старт с Windows',
  },
  {
    icon: InfinityIcon,
    title: 'Ring-буферы',
    text: 'std::array<float, 120> на каждый датчик. Память выделена один раз — куча не фрагментируется вообще.',
    tag: '0 аллокаций',
  },
  {
    icon: Zap,
    title: 'LTO + /O2',
    text: 'MSVC Release: полнопрограммная оптимизация, /GL + /LTCG, /OPT:REF/ICF — лишний код даже не попадает в бинарник.',
    tag: 'binary ~ 400 КБ',
  },
];

const STACK = ['WIN32 API', 'DIRECT2D 1.1', 'DWRITE', 'DXGI 1.4', 'PDH', 'DWM API', 'PRESENT HOOK', 'NTDLL', 'C++20', 'CMAKE', 'GITHUB ACTIONS'];

export default function Architecture() {
  return (
    <section id="architecture" className="relative scroll-mt-20 py-24 lg:py-32">
      <div className="mx-auto max-w-7xl px-5 lg:px-8">
        <SectionHead
          index="05"
          tag="ПОЧЕМУ ОН ЛЁГКИЙ"
          title={
            <>
              Инженерия
              <br />
              <span className="text-neon">вместо фреймворков.</span>
            </>
          }
          desc={
            <>
              Каждый мегабайт и каждая доля процента CPU здесь — осознанное архитектурное решение,
              а не случайность. Восемь принципов, на которых держится весь проект.
            </>
          }
        />

        <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
          {CARDS.map((c, i) => {
            const Icon = c.icon;
            return (
              <Reveal key={c.title} delay={(i % 4) * 90}>
                <div className="group relative h-full overflow-hidden rounded-2xl border border-white/8 bg-panel p-5 transition-all duration-300 hover:-translate-y-1 hover:border-neon/35 hover:shadow-[0_20px_60px_rgba(0,255,136,0.08)]">
                  <div className="mb-4 flex items-start justify-between">
                    <span className="grid h-10 w-10 place-items-center rounded-xl border border-white/10 bg-white/[0.03] text-neon transition-colors group-hover:border-neon/40 group-hover:bg-neon/10">
                      <Icon size={17} />
                    </span>
                    <span className="font-mono text-[9px] tracking-[0.2em] text-slate-600">
                      {String(i + 1).padStart(2, '0')}
                    </span>
                  </div>
                  <div className="font-mono text-[13px] font-bold text-white">{c.title}</div>
                  <p className="mt-2 min-h-[72px] text-[12px] leading-relaxed text-slate-400">{c.text}</p>
                  <div className="mt-3 font-mono text-[9.5px] tracking-[0.18em] text-neon/80">{c.tag}</div>
                </div>
              </Reveal>
            );
          })}
        </div>
      </div>

      {/* марки технологий */}
      <div className="mt-20 overflow-hidden border-y border-white/8 bg-panel/60 py-5">
        <div className="marquee-track gap-10">
          {[0, 1].map((dup) => (
            <div key={dup} className="flex shrink-0 items-center gap-10">
              {STACK.map((s) => (
                <span key={s} className="flex items-center gap-10 whitespace-nowrap">
                  <span className="font-display text-[15px] font-medium text-slate-500 transition-colors hover:text-neon">
                    {s}
                  </span>
                  <i className="h-1.5 w-1.5 rotate-45 bg-neon/40" />
                </span>
              ))}
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
