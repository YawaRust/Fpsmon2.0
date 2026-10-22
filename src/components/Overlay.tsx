import { memo } from 'react';
import {
  SENSOR_MAP, fmtValue, hexToRgba, resolveColor, toLines,
  type Block, type Item, type OverlaySettings,
} from '../lib/types';
import type { SimSnapshot } from '../lib/sim';

/* ---------------- Sparkline ---------------- */

const Sparkline = memo(function Sparkline({
  data, color, w, h,
}: { data: number[]; color: string; w: number; h: number }) {
  const n = data.length;
  let max = 0;
  for (const v of data) if (v > max) max = v;
  if (max < 1) max = 100;
  max *= 1.12;

  const stepX = n > 1 ? w / (n - 1) : w;
  let pts = '';
  for (let i = 0; i < n; i++) {
    pts += `${(i * stepX).toFixed(1)},${(h - 1 - Math.min(1, data[i] / max) * (h - 2)).toFixed(1)} `;
  }

  return (
    <svg width={w} height={h} viewBox={`0 0 ${w} ${h}`} preserveAspectRatio="none" className="block shrink-0" aria-hidden>
      <rect x="0" y="0" width={w} height={h} rx="2" fill="rgba(255,255,255,0.07)" />
      <polyline points={pts} fill="none" stroke={color} strokeWidth="1.3" strokeLinejoin="round" strokeLinecap="round" />
    </svg>
  );
});

/* ---------------- показатель ---------------- */

function ItemView({
  item, snap, s, highlight,
}: { item: Item; snap: SimSnapshot; s: OverlaySettings; highlight?: boolean }) {
  const f = s.fontSize;
  const meta = SENSOR_MAP[item.sensor];
  const value = snap.values[item.sensor] ?? 0;
  const color = resolveColor(item, meta, value);
  const vSize = item.big ? f * 1.42 : f * 0.98;

  // разбивка FPS при включённой генерации кадров: 118 (59×2)
  const fgActive = s.fgDetect && snap.fg >= 2;
  const showBreak = item.fgBreakdown && fgActive && item.sensor === 'FPS';

  return (
    <span
      className="inline-flex items-baseline"
      style={{
        gap: f * 0.28,
        outline: highlight ? '1px dashed rgba(0,255,136,0.85)' : undefined,
        outlineOffset: 2,
        borderRadius: 3,
      }}
    >
      {item.showLabel && item.label ? (
        <span
          style={{
            color: s.labelColor,
            opacity: 0.78,
            fontSize: f * 0.68,
            letterSpacing: `${s.letterSpacing}em`,
            textTransform: s.uppercase ? 'uppercase' : 'none',
            whiteSpace: 'nowrap',
          }}
        >
          {item.label}
        </span>
      ) : null}

      <span
        style={{
          color,
          fontWeight: 700,
          fontSize: vSize,
          fontVariantNumeric: 'tabular-nums',
          whiteSpace: 'nowrap',
          lineHeight: 1,
        }}
      >
        {fmtValue(value, meta.dec)}
        {item.showUnit && meta.unit ? (
          <span style={{ opacity: 0.45, fontSize: '0.68em', marginLeft: 2 }}>{meta.unit}</span>
        ) : null}
      </span>

      {showBreak ? (
        <span
          style={{
            fontSize: f * 0.62,
            color: '#ff9dff',
            whiteSpace: 'nowrap',
            fontVariantNumeric: 'tabular-nums',
          }}
          title="нативные кадры × множитель генерации"
        >
          ({Math.round(snap.values.FPSNative)}×{snap.fg})
        </span>
      ) : null}

      {item.showGraph ? (
        <Sparkline
          data={snap.history[item.sensor] ?? []}
          color={item.color}
          w={Math.round(f * 3.6)}
          h={Math.max(11, Math.round(f * 0.82))}
        />
      ) : null}
    </span>
  );
}

/* ---------------- блок ---------------- */

function BlockView({
  block, snap, s, selectedItem, selectedBlock,
}: {
  block: Block; snap: SimSnapshot; s: OverlaySettings;
  selectedItem?: string | null; selectedBlock?: string | null;
}) {
  const f = s.fontSize;
  const lines = toLines(block.items);
  const active = selectedBlock === block.id;

  const titleBase: React.CSSProperties = {
    fontSize: f * 0.6,
    letterSpacing: `${Math.max(s.letterSpacing, 0.12)}em`,
    fontWeight: 700,
    textTransform: s.uppercase ? 'uppercase' : 'none',
    whiteSpace: 'nowrap',
    color: s.accentColor,
  };

  const titleNode =
    s.titleStyle === 'chip' ? (
      <span
        style={{
          ...titleBase,
          background: hexToRgba(s.accentColor, 0.16),
          padding: `${f * 0.1}px ${f * 0.32}px`,
          borderRadius: 4,
          display: 'inline-block',
        }}
      >
        {block.title}
      </span>
    ) : s.titleStyle === 'bar' ? (
      <span style={{ display: 'inline-flex', alignItems: 'center', gap: f * 0.28 }}>
        <span style={{ width: f * 0.2, height: f * 0.6, background: s.accentColor, display: 'inline-block' }} />
        <span style={titleBase}>{block.title}</span>
      </span>
    ) : (
      <span style={{ ...titleBase, opacity: 0.72 }}>{block.title}</span>
    );

  return (
    <div style={{ outline: active ? '1px dashed rgba(0,216,255,0.8)' : undefined, outlineOffset: 3, borderRadius: 4 }}>
      {block.showTitle && block.title ? (
        <div style={{ marginBottom: f * 0.2 }}>{titleNode}</div>
      ) : null}

      <div style={{ display: 'flex', flexDirection: 'column', gap: s.rowGap }}>
        {lines.map((line, li) => (
          <div key={li} style={{ display: 'flex', alignItems: 'baseline', gap: f * 0.62, flexWrap: 'nowrap' }}>
            {line.map((it) => (
              <ItemView key={it.uid} item={it} snap={snap} s={s} highlight={selectedItem === it.uid} />
            ))}
          </div>
        ))}
      </div>
    </div>
  );
}

/* ---------------- оверлей ---------------- */

export default function Overlay({
  snap, settings, selectedItem, selectedBlock,
}: {
  snap: SimSnapshot;
  settings: OverlaySettings;
  selectedItem?: string | null;
  selectedBlock?: string | null;
}) {
  const s = settings;
  const blocks = s.blocks.filter((b) => b.enabled && b.items.length > 0);
  const row = s.direction === 'row';
  const glass = s.theme === 'glass';

  return (
    <div
      style={{
        opacity: s.opacity,
        padding: s.padding,
        borderRadius: s.rounded ? s.cornerRadius : 0,
        border: s.showBorder ? `1px solid ${hexToRgba(s.borderColor, 0.3)}` : '1px solid transparent',
        background: hexToRgba(s.bgColor, s.bgOpacity),
        backdropFilter: glass ? 'blur(14px) saturate(1.3)' : undefined,
        WebkitBackdropFilter: glass ? 'blur(14px) saturate(1.3)' : undefined,
        boxShadow: s.shadow ? '0 10px 40px rgba(0,0,0,0.5)' : undefined,
        textShadow: !s.showBorder && s.bgOpacity < 0.25 ? '0 1px 3px rgba(0,0,0,0.95)' : undefined,
        fontFamily: s.fontFamily,
        fontWeight: s.fontBold ? 700 : 500,
        display: 'flex',
        flexDirection: row ? 'row' : 'column',
        alignItems: row ? 'center' : 'stretch',
        gap: s.blockGap,
        lineHeight: 1,
        userSelect: 'none',
        width: 'max-content',
      }}
    >
      {blocks.length === 0 ? (
        <div style={{ fontSize: s.fontSize * 0.75, color: 'rgba(203,213,225,0.4)' }}>все блоки скрыты</div>
      ) : (
        blocks.map((b, i) => (
          <div
            key={b.id}
            style={{
              display: 'flex',
              flexDirection: row ? 'row' : 'column',
              alignItems: row ? 'center' : 'stretch',
              gap: s.blockGap,
            }}
          >
            {i > 0 && s.showSeparators ? (
              <div
                style={
                  row
                    ? { width: 1, alignSelf: 'stretch', background: hexToRgba(s.borderColor, 0.25) }
                    : { height: 1, background: hexToRgba(s.borderColor, 0.18) }
                }
              />
            ) : null}
            <BlockView block={b} snap={snap} s={s} selectedItem={selectedItem} selectedBlock={selectedBlock} />
          </div>
        ))
      )}
    </div>
  );
}
