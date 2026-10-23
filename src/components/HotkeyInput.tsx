import { useEffect, useRef, useState } from 'react';
import { Keyboard, X } from 'lucide-react';
import { hotkeyLabel, type Hotkey } from '../lib/types';

const FORBIDDEN = new Set(['ControlLeft', 'ControlRight', 'ShiftLeft', 'ShiftRight', 'AltLeft', 'AltRight', 'MetaLeft', 'MetaRight', 'Tab']);

export default function HotkeyInput({
  value, onChange, conflict,
}: {
  value: Hotkey;
  onChange: (h: Hotkey) => void;
  conflict?: boolean;
}) {
  const [listening, setListening] = useState(false);
  const ref = useRef<HTMLButtonElement | null>(null);

  useEffect(() => {
    if (!listening) return;

    const onKey = (e: KeyboardEvent) => {
      e.preventDefault();
      e.stopPropagation();

      if (e.code === 'Escape') {
        setListening(false);
        return;
      }
      if (FORBIDDEN.has(e.code)) return;

      onChange({ ctrl: e.ctrlKey, shift: e.shiftKey, alt: e.altKey, code: e.code });
      setListening(false);
    };

    const onBlur = () => setListening(false);

    window.addEventListener('keydown', onKey, true);
    window.addEventListener('blur', onBlur);
    return () => {
      window.removeEventListener('keydown', onKey, true);
      window.removeEventListener('blur', onBlur);
    };
  }, [listening, onChange]);

  return (
    <button
      ref={ref}
      type="button"
      onClick={() => setListening((v) => !v)}
      className={`flex min-w-[108px] items-center justify-center gap-1.5 rounded border px-2 py-[5px] font-mono text-[9.5px] transition-colors ${
        listening
          ? 'animate-pulse border-neon bg-neon/15 text-neon'
          : conflict
            ? 'border-danger/60 bg-danger/10 text-danger'
            : 'border-white/12 bg-white/[0.04] text-slate-200 hover:border-neon/45 hover:text-neon'
      }`}
      title={listening ? 'Нажмите комбинацию · Esc — отмена' : 'Назначить комбинацию'}
    >
      {listening ? (
        <>
          <Keyboard size={10} className="animate-blink" />
          нажмите…
        </>
      ) : conflict ? (
        <>
          <X size={10} />
          {hotkeyLabel(value)}
        </>
      ) : (
        hotkeyLabel(value)
      )}
    </button>
  );
}
