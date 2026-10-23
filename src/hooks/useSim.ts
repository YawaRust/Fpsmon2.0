import { useEffect, useState } from 'react';
import { SimEngine, type SimSnapshot } from '../lib/sim';

/** rAF-цикл движка + обновление React-снапшота ~10 Гц */
export function useSim(engine: SimEngine): SimSnapshot {
  const [snap, setSnap] = useState<SimSnapshot>(() => engine.snapshot());

  useEffect(() => {
    let raf = 0;
    let last = performance.now();
    let lastSet = 0;

    const loop = (t: number) => {
      const dt = Math.min(64, t - last);
      last = t;
      engine.step(dt);
      if (t - lastSet > 100) {
        lastSet = t;
        setSnap(engine.snapshot());
      }
      raf = requestAnimationFrame(loop);
    };

    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [engine]);

  return snap;
}
