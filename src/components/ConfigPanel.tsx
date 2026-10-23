import { useState } from 'react';
import {
  Check, ChevronDown, ChevronUp, Columns3, Cpu, Download, Eye, EyeOff, Gauge, HardDrive,
  Keyboard, Layers, type LucideIcon, MemoryStick, Minus, MonitorCog, Palette, Plus, Power,
  RotateCcw, Rows3, Settings2, Sparkles, Trash2, Type, Video, VideoOff, X,
} from 'lucide-react';
import HotkeyInput from './HotkeyInput';
import {
  HOTKEY_ACTIONS, HW_META, SENSORS, SENSOR_MAP, defaultSettings, hotkeyEquals, makeBlock,
  makeItem, moveIn,
  type Block, type HW, type HotkeyAction, type Item, type OverlayPos, type OverlaySettings,
  type SensorId,
} from '../lib/types';
import { THEMES, applyTheme } from '../lib/themes';
import { downloadIni } from '../lib/exportIni';

type Tab = 'layout' | 'theme' | 'style' | 'keys' | 'system';

const HW_ICON: Record<HW, LucideIcon> = {
  fps: Gauge, cpu: Cpu, gpu: MonitorCog, ram: MemoryStick, io: HardDrive,
};

/* ================= контролы ================= */

function IconBtn({
  on, onClick, title, children, danger, disabled,
}: {
  on?: boolean; onClick: () => void; title: string;
  children: React.ReactNode; danger?: boolean; disabled?: boolean;
}) {
  return (
    <button
      type="button" title={title} disabled={disabled} onClick={onClick}
      className={`grid h-[22px] w-[22px] shrink-0 place-items-center rounded border transition-colors ${
        disabled ? 'cursor-not-allowed border-white/5 text-slate-700'
          : danger ? 'border-white/10 text-slate-500 hover:border-danger/60 hover:text-danger'
          : on ? 'border-neon/60 bg-neon/15 text-neon'
          : 'border-white/10 text-slate-500 hover:border-white/30 hover:text-slate-200'
      }`}
    >
      {children}
    </button>
  );
}

function MiniSlider({
  label, value, min, max, step = 1, suffix = '', onChange,
}: { label: string; value: number; min: number; max: number; step?: number; suffix?: string; onChange: (v: number) => void }) {
  return (
    <label className="block py-1.5">
      <span className="mb-1 flex items-baseline justify-between font-mono text-[10px]">
        <span className="text-slate-400">{label}</span>
        <span className="text-neon tabular-nums">{value}{suffix}</span>
      </span>
      <input type="range" min={min} max={max} step={step} value={value}
        onChange={(e) => onChange(Number(e.target.value))} className="slider w-full" />
    </label>
  );
}

function MiniToggle({
  label, checked, onChange, hint,
}: { label: string; checked: boolean; onChange: (v: boolean) => void; hint?: string }) {
  return (
    <button type="button" onClick={() => onChange(!checked)}
      className="group flex w-full items-center justify-between gap-3 py-1.5 text-left" title={hint}>
      <span className="font-mono text-[10.5px] text-slate-300 group-hover:text-white">{label}</span>
      <span className={`relative h-[16px] w-[30px] shrink-0 rounded-full transition-colors ${checked ? 'bg-neon' : 'bg-white/12'}`}>
        <span className={`absolute top-[2px] h-[12px] w-[12px] rounded-full transition-all ${checked ? 'left-[16px] bg-ink' : 'left-[2px] bg-slate-400'}`} />
      </span>
    </button>
  );
}

/* ================= выбор датчика ================= */

function SensorPicker({ onPick, onClose }: { onPick: (id: SensorId) => void; onClose: () => void }) {
  const [hw, setHw] = useState<HW>('cpu');
  return (
    <div className="mt-1.5 rounded-lg border border-white/12 bg-black/50 p-2">
      <div className="mb-2 flex items-center gap-1">
        {(Object.keys(HW_META) as HW[]).map((k) => {
          const Icon = HW_ICON[k];
          return (
            <button key={k} onClick={() => setHw(k)} title={HW_META[k].label}
              className={`grid h-[22px] flex-1 place-items-center rounded border transition-colors ${
                hw === k ? 'border-neon/60 bg-neon/12 text-neon' : 'border-white/8 text-slate-500 hover:text-slate-200'
              }`}>
              <Icon size={12} />
            </button>
          );
        })}
        <IconBtn onClick={onClose} title="Закрыть"><X size={12} /></IconBtn>
      </div>
      <div className="grid max-h-[130px] grid-cols-2 gap-1 overflow-y-auto">
        {SENSORS.filter((s) => s.hw === hw).map((s) => (
          <button key={s.id} onClick={() => onPick(s.id)}
            className="flex items-center gap-1.5 rounded border border-white/8 px-2 py-1.5 text-left font-mono text-[9.5px] text-slate-300 transition-colors hover:border-neon/50 hover:bg-neon/[0.07] hover:text-neon">
            <span className="h-1.5 w-1.5 shrink-0 rounded-full" style={{ background: s.color }} />
            <span className="truncate">{s.name}</span>
            {s.fgOnly && <span className="shrink-0 text-[7.5px] text-fuchsia-400">FG</span>}
            <span className="ml-auto shrink-0 text-slate-600">{s.unit || '—'}</span>
          </button>
        ))}
      </div>
    </div>
  );
}

/* ================= строка показателя ================= */

function ItemRow({
  item, index, total, onPatch, onMove, onDelete, onSelect,
}: {
  item: Item; index: number; total: number;
  onPatch: (p: Partial<Item>) => void; onMove: (d: number) => void;
  onDelete: () => void; onSelect: () => void;
}) {
  const [open, setOpen] = useState(false);
  const meta = SENSOR_MAP[item.sensor];

  return (
    <div className="rounded-md border border-white/[0.07] bg-white/[0.02] px-1.5 py-1.5 transition-colors hover:border-white/15" onMouseEnter={onSelect}>
      <div className="flex items-center gap-1">
        <div className="flex flex-col">
          <button onClick={() => onMove(-1)} disabled={index === 0} title="Выше"
            className="grid h-[11px] w-[16px] place-items-center text-slate-600 transition-colors hover:text-neon disabled:text-slate-800">
            <ChevronUp size={11} />
          </button>
          <button onClick={() => onMove(1)} disabled={index === total - 1} title="Ниже"
            className="grid h-[11px] w-[16px] place-items-center text-slate-600 transition-colors hover:text-neon disabled:text-slate-800">
            <ChevronDown size={11} />
          </button>
        </div>

        <IconBtn on={item.sameLine} disabled={index === 0}
          onClick={() => onPatch({ sameLine: !item.sameLine })}
          title={item.sameLine ? 'Отделить на новую строку' : 'Присоединить к строке выше'}>
          {item.sameLine ? <Minus size={11} /> : <Rows3 size={11} />}
        </IconBtn>

        <input value={item.label} placeholder="без имени"
          onChange={(e) => onPatch({ label: e.target.value, showLabel: e.target.value.length > 0 })}
          className="min-w-0 flex-1 rounded border border-transparent bg-white/[0.04] px-1.5 py-[3px] font-mono text-[10px] text-slate-100 outline-none transition-colors placeholder:text-slate-600 focus:border-neon/50"
          title="Своё имя показателя (пустое = только значение)" />

        <span className="shrink-0 font-mono text-[8.5px] text-slate-600">{meta.unit || '—'}</span>

        <input type="color" value={item.color} className="colorwell !h-[19px] !w-[19px]"
          onChange={(e) => onPatch({ color: e.target.value })} title="Цвет значения" />

        <IconBtn on={item.big} onClick={() => onPatch({ big: !item.big })} title="Крупный шрифт"><Type size={11} /></IconBtn>
        <IconBtn on={item.showGraph} onClick={() => onPatch({ showGraph: !item.showGraph })} title="Мини-график"><Sparkles size={11} /></IconBtn>
        <IconBtn on={open} onClick={() => setOpen((v) => !v)} title="Пороги и единицы"><Settings2 size={11} /></IconBtn>
        <IconBtn danger onClick={onDelete} title="Удалить"><Trash2 size={11} /></IconBtn>
      </div>

      {open && (
        <div className="mt-1.5 flex flex-wrap items-center gap-2 border-t border-white/8 pt-1.5 pl-[18px]">
          <label className="flex items-center gap-1 font-mono text-[9px] text-slate-500">
            ПРЕД.
            <input type="number" className="mini-num !w-[42px]" value={item.warning || ''} placeholder="—"
              onChange={(e) => onPatch({ warning: Math.max(0, Math.round(e.target.valueAsNumber || 0)) })} />
          </label>
          <label className="flex items-center gap-1 font-mono text-[9px] text-slate-500">
            КРИТ.
            <input type="number" className="mini-num !w-[42px]" value={item.critical || ''} placeholder="—"
              onChange={(e) => onPatch({ critical: Math.max(0, Math.round(e.target.valueAsNumber || 0)) })} />
          </label>
          <button onClick={() => onPatch({ showUnit: !item.showUnit })}
            className={`rounded border px-1.5 py-[2px] font-mono text-[9px] transition-colors ${item.showUnit ? 'border-neon/50 bg-neon/10 text-neon' : 'border-white/10 text-slate-500'}`}>
            ед. изм.
          </button>
          <button onClick={() => onPatch({ showLabel: !item.showLabel })}
            className={`rounded border px-1.5 py-[2px] font-mono text-[9px] transition-colors ${item.showLabel ? 'border-neon/50 bg-neon/10 text-neon' : 'border-white/10 text-slate-500'}`}>
            имя
          </button>
          {item.sensor === 'FPS' && (
            <button onClick={() => onPatch({ fgBreakdown: !item.fgBreakdown })}
              className={`rounded border px-1.5 py-[2px] font-mono text-[9px] transition-colors ${item.fgBreakdown ? 'border-fuchsia-400/60 bg-fuchsia-400/10 text-fuchsia-300' : 'border-white/10 text-slate-500'}`}
              title="Показывать (нативные×множитель) при включённом фреймгене">
              разбивка FG
            </button>
          )}
        </div>
      )}
    </div>
  );
}

/* ================= блок ================= */

function BlockCard({
  block, index, total, onPatch, onMove, onDelete, onItems, onSelectBlock, onSelectItem,
}: {
  block: Block; index: number; total: number;
  onPatch: (p: Partial<Block>) => void; onMove: (d: number) => void; onDelete: () => void;
  onItems: (items: Item[]) => void; onSelectBlock: () => void; onSelectItem: (uid: string | null) => void;
}) {
  const [adding, setAdding] = useState(false);
  const [collapsed, setCollapsed] = useState(false);
  const Icon = HW_ICON[block.hw];
  const accent = HW_META[block.hw].color;

  return (
    <div className={`rounded-lg border bg-black/25 transition-colors ${block.enabled ? 'border-white/12' : 'border-white/[0.06] opacity-55'}`} onMouseEnter={onSelectBlock}>
      <div className="flex items-center gap-1 px-1.5 py-1.5">
        <div className="flex flex-col">
          <button onClick={() => onMove(-1)} disabled={index === 0} title="Блок выше"
            className="grid h-[11px] w-[16px] place-items-center text-slate-600 hover:text-cyber disabled:text-slate-800">
            <ChevronUp size={11} />
          </button>
          <button onClick={() => onMove(1)} disabled={index === total - 1} title="Блок ниже"
            className="grid h-[11px] w-[16px] place-items-center text-slate-600 hover:text-cyber disabled:text-slate-800">
            <ChevronDown size={11} />
          </button>
        </div>

        <span className="grid h-[22px] w-[22px] shrink-0 place-items-center rounded" style={{ background: `${accent}1f`, color: accent }}>
          <Icon size={12} />
        </span>

        <input value={block.title} placeholder="без заголовка"
          onChange={(e) => onPatch({ title: e.target.value, showTitle: e.target.value.length > 0 })}
          className="min-w-0 flex-1 rounded border border-transparent bg-white/[0.04] px-1.5 py-[3px] font-mono text-[10.5px] font-bold tracking-wider text-slate-100 outline-none placeholder:font-normal placeholder:tracking-normal placeholder:text-slate-600 focus:border-cyber/50"
          title="Название блока (пустое = скрыт)" />

        <IconBtn on={block.showTitle} onClick={() => onPatch({ showTitle: !block.showTitle })} title="Показывать заголовок">
          {block.showTitle ? <Eye size={11} /> : <EyeOff size={11} />}
        </IconBtn>
        <IconBtn
          on={block.items.length > 1 && block.items.slice(1).every((i) => i.sameLine)}
          onClick={() => {
            const all = block.items.slice(1).every((i) => i.sameLine);
            onItems(block.items.map((it, i) => (i === 0 ? it : { ...it, sameLine: !all })));
          }}
          title="Собрать блок в одну строку / разложить">
          <Columns3 size={11} />
        </IconBtn>
        <IconBtn on={block.enabled} onClick={() => onPatch({ enabled: !block.enabled })} title="Показывать блок"><Layers size={11} /></IconBtn>
        <IconBtn danger onClick={onDelete} title="Удалить блок"><Trash2 size={11} /></IconBtn>
        <button onClick={() => setCollapsed((v) => !v)} className="grid h-[22px] w-[18px] place-items-center text-slate-600 hover:text-white" title={collapsed ? 'Развернуть' : 'Свернуть'}>
          {collapsed ? <ChevronDown size={13} /> : <ChevronUp size={13} />}
        </button>
      </div>

      {!collapsed && (
        <div className="space-y-1 px-1.5 pb-1.5" onMouseLeave={() => onSelectItem(null)}>
          {block.items.map((it, i) => (
            <ItemRow key={it.uid} item={it} index={i} total={block.items.length}
              onSelect={() => onSelectItem(it.uid)}
              onPatch={(p) => onItems(block.items.map((x) => (x.uid === it.uid ? { ...x, ...p } : x)))}
              onMove={(d) => onItems(moveIn(block.items, i, d))}
              onDelete={() => onItems(block.items.filter((x) => x.uid !== it.uid))} />
          ))}

          {adding ? (
            <SensorPicker onClose={() => setAdding(false)} onPick={(sid) => { onItems([...block.items, makeItem(sid)]); setAdding(false); }} />
          ) : (
            <button onClick={() => setAdding(true)}
              className="flex w-full items-center justify-center gap-1.5 rounded-md border border-dashed border-white/12 py-1.5 font-mono text-[9.5px] text-slate-500 transition-colors hover:border-neon/40 hover:text-neon">
              <Plus size={11} /> показатель
            </button>
          )}
        </div>
      )}
    </div>
  );
}

/* ================= панель ================= */

const TABS: { id: Tab; label: string; icon: LucideIcon }[] = [
  { id: 'layout', label: 'БЛОКИ', icon: Layers },
  { id: 'theme', label: 'ТЕМЫ', icon: Palette },
  { id: 'style', label: 'СТИЛЬ', icon: Type },
  { id: 'keys', label: 'КЛАВИШИ', icon: Keyboard },
  { id: 'system', label: 'СИСТЕМА', icon: Settings2 },
];

const CORNERS: { id: Exclude<OverlayPos, 'custom'>; label: string }[] = [
  { id: 'tl', label: '◤ верх-лево' }, { id: 'tr', label: '◥ верх-право' },
  { id: 'bl', label: '◣ низ-лево' }, { id: 'br', label: '◢ низ-право' },
];

export default function ConfigPanel({
  settings, setSettings, onClose, onSelectBlock, onSelectItem, cpuCost, maxHeight, brand,
}: {
  settings: OverlaySettings;
  setSettings: React.Dispatch<React.SetStateAction<OverlaySettings>>;
  onClose: () => void;
  onSelectBlock: (id: string | null) => void;
  onSelectItem: (uid: string | null) => void;
  cpuCost: { ram: string; cpu: string };
  maxHeight: number;
  brand: string;
}) {
  const [tab, setTab] = useState<Tab>('layout');
  const [addingBlock, setAddingBlock] = useState(false);
  const s = settings;
  const patch = (p: Partial<OverlaySettings>) => setSettings((v) => ({ ...v, ...p }));
  const setBlocks = (blocks: Block[]) => patch({ blocks });

  const conflicts = new Set<HotkeyAction>();
  (Object.keys(s.hotkeys) as HotkeyAction[]).forEach((a) => {
    (Object.keys(s.hotkeys) as HotkeyAction[]).forEach((b) => {
      if (a !== b && hotkeyEquals(s.hotkeys[a], s.hotkeys[b])) conflicts.add(a);
    });
  });

  return (
    <div
      className="flex w-[338px] flex-col overflow-hidden rounded-xl border border-neon/25 bg-[#05070b]/95 shadow-[0_24px_70px_rgba(0,0,0,0.7)] backdrop-blur-md"
      style={{ fontFamily: '"JetBrains Mono", monospace', maxHeight: Math.max(240, maxHeight) }}
    >
      <div data-drag-handle className="flex cursor-move items-center gap-2 border-b border-white/10 bg-white/[0.03] px-3 py-2">
        <span className="h-1.5 w-1.5 animate-blink rounded-full bg-neon" />
        <span className="truncate font-mono text-[10px] tracking-[0.18em] text-neon">{brand.toUpperCase()}</span>
        <span className="rounded border border-white/12 px-1 py-[1px] font-mono text-[8px] text-slate-500">0.0.1</span>
        <button onClick={onClose} className="ml-auto grid h-[20px] w-[20px] place-items-center rounded text-slate-500 transition-colors hover:bg-white/10 hover:text-white" title="Закрыть (Esc)">
          <X size={12} />
        </button>
      </div>

      <div className="flex border-b border-white/8">
        {TABS.map((t) => {
          const Icon = t.icon;
          const on = tab === t.id;
          return (
            <button key={t.id} onClick={() => setTab(t.id)} title={t.label}
              className={`flex flex-1 items-center justify-center gap-1 py-2 font-mono text-[8.5px] tracking-[0.1em] transition-colors ${
                on ? 'bg-neon/[0.08] text-neon shadow-[inset_0_-2px_0_0_rgba(0,255,136,0.9)]' : 'text-slate-500 hover:text-slate-200'
              }`}>
              <Icon size={11} />
              <span className="hidden xl:inline">{t.label}</span>
            </button>
          );
        })}
      </div>

      <div className="min-h-0 flex-1 overflow-y-auto p-2.5">
        {/* ---------- БЛОКИ ---------- */}
        {tab === 'layout' && (
          <div className="space-y-2" onMouseLeave={() => { onSelectBlock(null); onSelectItem(null); }}>
            {s.blocks.map((b, i) => (
              <BlockCard key={b.id} block={b} index={i} total={s.blocks.length}
                onSelectBlock={() => onSelectBlock(b.id)} onSelectItem={onSelectItem}
                onPatch={(p) => setBlocks(s.blocks.map((x) => (x.id === b.id ? { ...x, ...p } : x)))}
                onMove={(d) => setBlocks(moveIn(s.blocks, i, d))}
                onDelete={() => setBlocks(s.blocks.filter((x) => x.id !== b.id))}
                onItems={(items) => setBlocks(s.blocks.map((x) => (x.id === b.id ? { ...x, items } : x)))} />
            ))}

            {addingBlock ? (
              <div className="rounded-lg border border-white/12 bg-black/40 p-2">
                <div className="mb-1.5 font-mono text-[9px] tracking-[0.2em] text-slate-500">НОВЫЙ БЛОК</div>
                <div className="grid grid-cols-2 gap-1">
                  {(Object.keys(HW_META) as HW[]).map((k) => {
                    const Icon = HW_ICON[k];
                    return (
                      <button key={k} onClick={() => {
                        const first = SENSORS.find((x) => x.hw === k)!;
                        setBlocks([...s.blocks, makeBlock(k, HW_META[k].label.toUpperCase(), [makeItem(first.id)])]);
                        setAddingBlock(false);
                      }}
                        className="flex items-center gap-1.5 rounded border border-white/10 px-2 py-1.5 font-mono text-[9.5px] text-slate-300 transition-colors hover:border-neon/50 hover:text-neon">
                        <Icon size={11} style={{ color: HW_META[k].color }} />
                        {HW_META[k].label}
                      </button>
                    );
                  })}
                  <button onClick={() => setAddingBlock(false)} className="rounded border border-white/10 px-2 py-1.5 font-mono text-[9.5px] text-slate-500 hover:text-white">отмена</button>
                </div>
              </div>
            ) : (
              <button onClick={() => setAddingBlock(true)}
                className="flex w-full items-center justify-center gap-1.5 rounded-lg border border-dashed border-white/15 py-2 font-mono text-[10px] text-slate-400 transition-colors hover:border-neon/50 hover:text-neon">
                <Plus size={12} /> добавить блок
              </button>
            )}
          </div>
        )}

        {/* ---------- ТЕМЫ ---------- */}
        {tab === 'theme' && (
          <div>
            <div className="mb-2 font-mono text-[9px] tracking-[0.2em] text-slate-500">ГОТОВЫЕ СТИЛИ ОВЕРЛЕЯ</div>
            <div className="space-y-1.5">
              {THEMES.map((t) => {
                const on = s.theme === t.id;
                const p = t.style.palette;
                return (
                  <button key={t.id} onClick={() => setSettings((v) => applyTheme(v, t.id))}
                    className={`flex w-full items-center gap-2.5 rounded-lg border p-2 text-left transition-all ${
                      on ? 'border-neon/60 bg-neon/[0.07]' : 'border-white/10 hover:border-white/25'
                    }`}>
                    {/* мини-превью */}
                    <span
                      className="grid h-[34px] w-[46px] shrink-0 place-items-center overflow-hidden rounded"
                      style={{
                        background: t.style.bgOpacity > 0.05 ? t.style.bgColor : '#1b2230',
                        border: t.style.showBorder ? `1px solid ${t.style.borderColor}55` : '1px solid transparent',
                        borderRadius: t.style.rounded ? 5 : 1,
                      }}>
                      <span className="flex flex-col gap-[2px]">
                        <span className="flex gap-[2px]">
                          <i className="block h-[3px] w-[11px] rounded-sm" style={{ background: p.fps }} />
                          <i className="block h-[3px] w-[6px] rounded-sm" style={{ background: p.cpu }} />
                        </span>
                        <span className="flex gap-[2px]">
                          <i className="block h-[3px] w-[7px] rounded-sm" style={{ background: p.gpu }} />
                          <i className="block h-[3px] w-[9px] rounded-sm" style={{ background: p.ram }} />
                        </span>
                        <span className="block h-[3px] w-[14px] rounded-sm" style={{ background: p.io, opacity: 0.7 }} />
                      </span>
                    </span>
                    <span className="min-w-0 flex-1">
                      <span className={`block font-mono text-[10.5px] font-bold ${on ? 'text-neon' : 'text-slate-200'}`}>{t.name}</span>
                      <span className="mt-0.5 block text-[9px] leading-snug text-slate-500">{t.desc}</span>
                    </span>
                    {on && <Check size={13} className="shrink-0 text-neon" />}
                  </button>
                );
              })}
            </div>
            <p className="mt-2 font-mono text-[8.5px] leading-relaxed text-slate-600">
              тема меняет шрифт, фон, рамку и палитру блоков. раскладка, имена и пороги сохраняются.
            </p>
          </div>
        )}

        {/* ---------- СТИЛЬ ---------- */}
        {tab === 'style' && (
          <div>
            <MiniSlider label="Размер шрифта" value={s.fontSize} min={9} max={28} suffix="px" onChange={(v) => patch({ fontSize: v })} />
            <MiniSlider label="Прозрачность" value={Math.round(s.opacity * 100)} min={20} max={100} suffix="%" onChange={(v) => patch({ opacity: v / 100 })} />
            <MiniSlider label="Заливка фона" value={Math.round(s.bgOpacity * 100)} min={0} max={100} suffix="%" onChange={(v) => patch({ bgOpacity: v / 100 })} />
            <MiniSlider label="Поля" value={s.padding} min={2} max={22} suffix="px" onChange={(v) => patch({ padding: v })} />
            <MiniSlider label="Между строками" value={s.rowGap} min={0} max={12} suffix="px" onChange={(v) => patch({ rowGap: v })} />
            <MiniSlider label="Между блоками" value={s.blockGap} min={0} max={24} suffix="px" onChange={(v) => patch({ blockGap: v })} />
            <MiniSlider label="Скругление" value={s.cornerRadius} min={0} max={20} suffix="px" onChange={(v) => patch({ cornerRadius: v, rounded: v > 0 })} />

            <div className="my-2 flex items-center gap-2 border-t border-white/8 pt-2">
              {([['Фон', 'bgColor'], ['Рамка', 'borderColor'], ['Имена', 'labelColor'], ['Акцент', 'accentColor']] as const).map(([lbl, key]) => (
                <label key={key} className="flex flex-1 flex-col items-center gap-1">
                  <input type="color" value={s[key]} className="colorwell" onChange={(e) => patch({ [key]: e.target.value } as Partial<OverlaySettings>)} />
                  <span className="font-mono text-[8px] text-slate-500">{lbl}</span>
                </label>
              ))}
            </div>

            <div className="mb-1.5 font-mono text-[9px] tracking-[0.2em] text-slate-500">НАПРАВЛЕНИЕ</div>
            <div className="mb-2 grid grid-cols-2 gap-1.5">
              {([['column', 'столбцом', Rows3], ['row', 'строкой', Columns3]] as const).map(([dir, label, Icon]) => (
                <button key={dir} onClick={() => patch({ direction: dir })}
                  className={`flex items-center justify-center gap-1.5 rounded border py-1.5 font-mono text-[9.5px] transition-colors ${
                    s.direction === dir ? 'border-neon/60 bg-neon/10 text-neon' : 'border-white/10 text-slate-400 hover:text-white'
                  }`}>
                  <Icon size={11} /> {label}
                </button>
              ))}
            </div>

            <div className="mb-1.5 font-mono text-[9px] tracking-[0.2em] text-slate-500">ЗАГОЛОВКИ БЛОКОВ</div>
            <div className="mb-2 grid grid-cols-3 gap-1.5">
              {([['plain', 'обычные'], ['chip', 'плашка'], ['bar', 'полоса']] as const).map(([ts, label]) => (
                <button key={ts} onClick={() => patch({ titleStyle: ts })}
                  className={`rounded border py-1.5 font-mono text-[9px] transition-colors ${
                    s.titleStyle === ts ? 'border-neon/60 bg-neon/10 text-neon' : 'border-white/10 text-slate-400 hover:text-white'
                  }`}>
                  {label}
                </button>
              ))}
            </div>

            <MiniToggle label="Рамка" checked={s.showBorder} onChange={(v) => patch({ showBorder: v })} />
            <MiniToggle label="Разделители" checked={s.showSeparators} onChange={(v) => patch({ showSeparators: v })} />
            <MiniToggle label="Жирный шрифт" checked={s.fontBold} onChange={(v) => patch({ fontBold: v })} />
            <MiniToggle label="ЗАГЛАВНЫЕ имена" checked={s.uppercase} onChange={(v) => patch({ uppercase: v })} />
            <MiniToggle label="Тень панели" checked={s.shadow} onChange={(v) => patch({ shadow: v })} />

            <div className="my-2 border-t border-white/8" />
            <div className="mb-1.5 font-mono text-[9px] tracking-[0.2em] text-slate-500">ПОЗИЦИЯ</div>
            <div className="grid grid-cols-2 gap-1.5">
              {CORNERS.map((c) => (
                <button key={c.id} onClick={() => patch({ position: c.id })}
                  className={`rounded border py-1.5 font-mono text-[9.5px] transition-colors ${
                    s.position === c.id ? 'border-neon/60 bg-neon/10 text-neon' : 'border-white/10 text-slate-400 hover:text-white'
                  }`}>
                  {c.label}
                </button>
              ))}
            </div>
          </div>
        )}

        {/* ---------- КЛАВИШИ ---------- */}
        {tab === 'keys' && (
          <div>
            <div className="mb-2 font-mono text-[9px] tracking-[0.2em] text-slate-500">СВОИ СОЧЕТАНИЯ · RegisterHotKey</div>
            <div className="space-y-1.5">
              {HOTKEY_ACTIONS.map((a) => (
                <div key={a.id} className="flex items-center gap-2 rounded-md border border-white/[0.07] bg-white/[0.02] px-2 py-1.5">
                  <span className="min-w-0 flex-1">
                    <span className="block truncate font-mono text-[10px] text-slate-200">{a.label}</span>
                    <span className="block font-mono text-[8px] text-slate-600">{a.hint}</span>
                  </span>
                  <HotkeyInput
                    value={s.hotkeys[a.id]}
                    conflict={conflicts.has(a.id)}
                    onChange={(h) => patch({ hotkeys: { ...s.hotkeys, [a.id]: h } })}
                  />
                </div>
              ))}
            </div>

            {conflicts.size > 0 && (
              <div className="mt-2 rounded-md border border-danger/40 bg-danger/10 p-2 font-mono text-[9px] leading-relaxed text-danger">
                обнаружены одинаковые сочетания — RegisterHotKey вернёт ошибку для второго действия.
              </div>
            )}

            <button
              onClick={() => patch({ hotkeys: defaultSettings().hotkeys })}
              className="mt-2 flex w-full items-center justify-center gap-1.5 rounded-md border border-white/12 py-1.5 font-mono text-[9.5px] text-slate-400 transition-colors hover:border-white/30 hover:text-white">
              <RotateCcw size={11} /> вернуть стандартные
            </button>

            <p className="mt-2 font-mono text-[8.5px] leading-relaxed text-slate-600">
              кликните по кнопке и нажмите нужную комбинацию. Esc — отмена. Клавиши-модификаторы
              отдельно не назначаются.
            </p>
          </div>
        )}

        {/* ---------- СИСТЕМА ---------- */}
        {tab === 'system' && (
          <div>
            {s.gpuNames.length > 0 && (
              <section className="mb-2 rounded-lg border border-neon/20 bg-neon/[0.04] p-2">
                <div className="mb-1.5 flex items-center justify-between">
                  <span className="font-mono text-[9px] tracking-[0.18em] text-slate-500">АКТИВНЫЙ GPU</span>
                  <span className="rounded border border-neon/20 px-1 py-0.5 font-mono text-[8px] text-neon">{s.gpuNames.length} обнаружено</span>
                </div>
                <select value={Math.min(s.gpuIndex, s.gpuNames.length - 1)}
                  onChange={(e) => patch({ gpuIndex: Number(e.target.value) })}
                  className="w-full rounded-md border border-white/12 bg-[#0b1118] px-2 py-2 font-mono text-[10px] text-slate-100 outline-none focus:border-neon/60">
                  {s.gpuNames.map((name, index) => (
                    <option key={`${name}-${index}`} value={index}>{name}</option>
                  ))}
                </select>
                <p className="mt-1.5 font-mono text-[8px] leading-relaxed text-slate-600">
                  Выбранный адаптер используется для показателей загрузки и видеопамяти.
                </p>
              </section>
            )}

            {/* захват OBS */}
            <div className="mb-1.5 font-mono text-[9px] tracking-[0.2em] text-slate-500">ЗАХВАТ СТОРОННИМИ ПРОГРАММАМИ</div>
            <div className="grid grid-cols-2 gap-1.5">
              {([
                ['allow', 'разрешён', Video, 'WDA_NONE — оверлей попадёт в OBS, Discord, скриншоты'],
                ['exclude', 'скрыт', VideoOff, 'WDA_EXCLUDEFROMCAPTURE — невидим для записи'],
              ] as const).map(([mode, label, Icon, hint]) => (
                <button key={mode} onClick={() => patch({ captureMode: mode })} title={hint}
                  className={`flex items-center justify-center gap-1.5 rounded border py-2 font-mono text-[9.5px] transition-colors ${
                    s.captureMode === mode ? 'border-neon/60 bg-neon/10 text-neon' : 'border-white/10 text-slate-400 hover:text-white'
                  }`}>
                  <Icon size={12} /> {label}
                </button>
              ))}
            </div>
            <p className="mt-1.5 font-mono text-[8.5px] leading-relaxed text-slate-600">
              {s.captureMode === 'allow'
                ? 'оверлей виден на стриме и в записи — удобно для контента и бенчмарк-видео.'
                : 'оверлей не попадает в захват: зрители видят чистую картинку игры.'}
            </p>

            <div className="my-2 border-t border-white/8" />

            {/* автозапуск */}
            <div className="mb-1 flex items-center gap-1.5">
              <Power size={11} className="text-neon" />
              <span className="font-mono text-[9px] tracking-[0.2em] text-slate-500">АВТОЗАПУСК С WINDOWS</span>
            </div>
            <MiniToggle label="Запускать при старте системы" checked={s.autostart} onChange={(v) => patch({ autostart: v })} />
            {s.autostart && (
              <div className="mt-1 rounded-md border border-white/10 bg-black/30 p-2">
                <div className="mb-1.5 grid grid-cols-2 gap-1.5">
                  {([
                    ['registry', 'реестр HKCU', 'без прав администратора'],
                    ['task', 'планировщик', 'запуск до входа, с правами'],
                  ] as const).map(([m, label, hint]) => (
                    <button key={m} onClick={() => patch({ autostartMethod: m })} title={hint}
                      className={`rounded border px-1.5 py-1.5 font-mono text-[9px] transition-colors ${
                        s.autostartMethod === m ? 'border-neon/60 bg-neon/10 text-neon' : 'border-white/10 text-slate-400 hover:text-white'
                      }`}>
                      {label}
                    </button>
                  ))}
                </div>
                <MiniSlider label="Задержка старта" value={s.autostartDelay} min={0} max={60} step={5} suffix=" с" onChange={(v) => patch({ autostartDelay: v })} />
                <MiniToggle label="Стартовать свёрнутым в трей" checked={s.autostartMinimized} onChange={(v) => patch({ autostartMinimized: v })} />
                <div className="mt-1.5 rounded border border-white/8 bg-black/40 p-1.5 font-mono text-[8px] leading-relaxed break-all text-slate-500">
                  {s.autostartMethod === 'registry'
                    ? `HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\${brand.replace(/\s+/g, '')} = "%APPDATA%\\..\\app.exe" --autostart${s.autostartMinimized ? ' --tray' : ''}`
                    : `schtasks /create /tn "${brand}" /tr "app.exe --autostart" /sc onlogon /delay 0000:${String(s.autostartDelay).padStart(2, '0')} /rl highest`}
                </div>
              </div>
            )}

            <div className="my-2 border-t border-white/8" />

            {/* фреймген */}
            <MiniToggle label="Определять генерацию кадров" checked={s.fgDetect} onChange={(v) => patch({ fgDetect: v })} hint="DLSS 3 / FSR 3 / XeSS-FG" />
            <p className="mb-1 font-mono text-[8.5px] leading-relaxed text-slate-600">
              различает нативные и сгенерированные кадры через present-хук, показывает
              итог, нативный FPS, множитель и задержку.
            </p>

            <div className="my-2 border-t border-white/8" />

            <MiniSlider label="Опрос железа" value={s.updateInterval} min={100} max={2000} step={50} suffix=" мс" onChange={(v) => patch({ updateInterval: v })} />
            <MiniSlider label="Обновление FPS" value={s.fpsInterval} min={100} max={1000} step={50} suffix=" мс" onChange={(v) => patch({ fpsInterval: v })} />
            <MiniToggle label="Сквозные клики" checked={s.clickThrough} onChange={(v) => patch({ clickThrough: v })} hint="WS_EX_TRANSPARENT" />
            <MiniToggle label="Поверх всех окон" checked={s.alwaysOnTop} onChange={(v) => patch({ alwaysOnTop: v })} />

            <div className="mt-2 rounded-lg border border-neon/20 bg-neon/[0.05] p-2.5">
              {[['ОЗУ процесса', `${cpuCost.ram} МБ`], ['CPU', `${cpuCost.cpu} %`], ['Процессов', '1'], ['Окон', '1 layered']].map(([k, v]) => (
                <div key={k} className="flex justify-between py-[2px] font-mono text-[9.5px]">
                  <span className="text-slate-500">{k}</span>
                  <span className="text-neon tabular-nums">{v}</span>
                </div>
              ))}
            </div>
          </div>
        )}
      </div>

      <div className="flex gap-1.5 border-t border-white/10 bg-white/[0.02] p-2">
        {!window.chrome?.webview && (
          <button onClick={() => downloadIni(s)}
            className="flex flex-1 items-center justify-center gap-1.5 rounded-md bg-neon px-3 py-2 font-mono text-[10px] font-bold text-ink transition-colors hover:bg-[#5bff9f]">
            <Download size={12} /> config.ini
          </button>
        )}
        <button onClick={() => setSettings(defaultSettings())}
          className="grid h-[32px] w-[32px] place-items-center rounded-md border border-white/12 text-slate-400 transition-colors hover:border-white/30 hover:text-white" title="Сбросить всё">
          <RotateCcw size={13} />
        </button>
      </div>
    </div>
  );
}
