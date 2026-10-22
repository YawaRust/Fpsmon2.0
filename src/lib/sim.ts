/* ------------------------------------------------------------------
 * Симуляция датчиков. Поддерживает генерацию кадров (DLSS 3 / FSR 3 /
 * XeSS-FG): различает нативные, сгенерированные и итоговые кадры,
 * считает задержку (Reflex / PCL).
 * ------------------------------------------------------------------ */

import { SENSORS, type SensorId } from './types';

export const HISTORY_SIZE = 120;
const PUSH_MS = 100;

export type FrameGenMode = 0 | 2 | 3 | 4;

export interface SimSnapshot {
  values: Record<SensorId, number>;
  history: Record<SensorId, number[]>;
  fg: FrameGenMode;
}

const ALL: SensorId[] = SENSORS.map((s) => s.id);
const VRAM_TOTAL = 12288;
const RAM_TOTAL = 32768;

const rand = (a: number, b: number) => a + Math.random() * (b - a);
const clamp = (v: number, a: number, b: number) => (v < a ? a : v > b ? b : v);

const TAU: Partial<Record<SensorId, number>> = {
  FPS: 200, FPSNative: 200, FPSGen: 240, FrameGenX: 120, Latency: 420,
  Frametime: 200, FPSAvg: 2600, FPS1Low: 1400,
  CPUUsage: 360, CPUTemp: 3600, CPUClock: 520, CPUPower: 700, CPUFan: 2600,
  GPUUsage: 380, GPUTemp: 4000, GPUClock: 560, GPUPower: 650, GPUFan: 2800,
  VRAM: 4200, VRAMPct: 4200,
  RAMUsage: 3200, RAMUsed: 3200, RAMFree: 3200,
  DiskRead: 280, DiskWrite: 280, DiskActive: 420,
  NetDown: 400, NetUp: 400,
};

export class SimEngine {
  sysInterval = 1000;
  fpsInterval = 500;
  /** 0 = выключено, 2/3/4 = множитель генерации кадров */
  frameGen: FrameGenMode = 0;

  private values = {} as Record<SensorId, number>;
  private targets = {} as Record<SensorId, number>;
  private history = {} as Record<SensorId, number[]>;
  private fpsWindow: number[] = [];

  private tSys = 0;
  private tFps = 0;
  private tPush = 0;
  private dipRemain = 0;
  private dipCooldown = 4000;
  private cpuSpike = 0;

  constructor() {
    for (const k of ALL) {
      this.history[k] = [];
      this.values[k] = 0;
      this.targets[k] = 0;
    }
    const t = this.targets;
    t.FPSNative = 96; t.FPS = 96; t.FPSGen = 0; t.FrameGenX = 1; t.Latency = 24;
    t.FPSAvg = 94; t.FPS1Low = 72; t.Frametime = 10.4;
    t.CPUUsage = 44; t.CPUTemp = 57; t.CPUClock = 4800; t.CPUPower = 68; t.CPUFan = 1350;
    t.GPUUsage = 78; t.GPUTemp = 66; t.GPUClock = 2475; t.GPUPower = 245; t.GPUFan = 1720;
    t.VRAM = 6420; t.VRAMPct = (6420 / VRAM_TOTAL) * 100;
    t.RAMUsage = 47; t.RAMUsed = 15400; t.RAMFree = RAM_TOTAL - 15400;
    t.DiskRead = 2; t.DiskWrite = 1; t.DiskActive = 6; t.NetDown = 140; t.NetUp = 28;

    for (const k of ALL) this.values[k] = this.targets[k] * rand(0.93, 1.07);
    for (let i = 0; i < HISTORY_SIZE; i++) this.push(rand(0.9, 1.06));
  }

  setIntervals(sys: number, fps: number) {
    this.sysInterval = sys;
    this.fpsInterval = fps;
  }

  setFrameGen(mode: FrameGenMode) {
    this.frameGen = mode;
    this.fpsWindow = [];
  }

  resetStats() {
    this.fpsWindow = [];
    this.targets.FPSAvg = this.values.FPS;
    this.targets.FPS1Low = this.values.FPS * 0.8;
  }

  step(dt: number) {
    this.tSys += dt;
    this.tFps += dt;
    this.tPush += dt;

    if (this.dipRemain > 0) this.dipRemain -= dt;
    else {
      this.dipCooldown -= dt;
      if (this.dipCooldown <= 0) {
        this.dipRemain = rand(650, 1500);
        this.dipCooldown = rand(7000, 14000);
      }
    }

    /* ---- кадры ---- */
    if (this.tFps >= this.fpsInterval) {
      this.tFps %= this.fpsInterval;
      const t = this.targets;

      // нативный рендер
      if (this.dipRemain > 0) t.FPSNative = rand(28, 46);
      else {
        const base = 92 + Math.sin(performance.now() / 5200) * 14;
        t.FPSNative = clamp(base + rand(-11, 17), 34, 180);
      }

      const fg = this.frameGen;
      if (fg >= 2) {
        // генератор кадров: чуть снижает нативный рендер (накладные расходы)
        t.FPSNative *= 0.93;
        const mul = fg * rand(0.965, 0.995); // не идеальное умножение
        t.FPS = t.FPSNative * mul;
        t.FPSGen = t.FPS - t.FPSNative;
        t.FrameGenX = fg;
        // задержка растёт: кадр придерживается для интерполяции
        t.Latency = clamp(1000 / t.FPSNative * 1.35 + (fg - 1) * 4.5 + rand(-1.5, 2.5), 8, 95);
      } else {
        t.FPS = t.FPSNative;
        t.FPSGen = 0;
        t.FrameGenX = 1;
        t.Latency = clamp(1000 / t.FPSNative * 1.15 + rand(-1.2, 2), 5, 90);
      }

      this.fpsWindow.push(t.FPS);
      if (this.fpsWindow.length > 60) this.fpsWindow.shift();
      const sorted = this.fpsWindow.slice().sort((a, b) => a - b);
      t.FPS1Low = sorted[Math.floor(sorted.length * 0.01)] ?? sorted[0];
      t.FPSAvg = this.fpsWindow.reduce((s, v) => s + v, 0) / this.fpsWindow.length;
    }

    /* ---- железо ---- */
    if (this.cpuSpike > 0) this.cpuSpike -= dt;
    else if (Math.random() < dt / 13000) this.cpuSpike = rand(600, 1400);

    if (this.tSys >= this.sysInterval) {
      this.tSys %= this.sysInterval;
      const t = this.targets;
      const nat = t.FPSNative;

      let cpu = 24 + (nat / 96) * 30 + rand(-6, 9);
      if (this.cpuSpike > 0) cpu += rand(34, 54);
      t.CPUUsage = clamp(cpu, 5, 99);

      // фреймген нагружает GPU сильнее (optical flow / tensor)
      const fgLoad = this.frameGen >= 2 ? 6 + this.frameGen * 2.2 : 0;
      t.GPUUsage = clamp(58 + (nat / 96) * 32 + fgLoad + rand(-5, 6), 20, 99);

      t.CPUTemp = clamp(41 + t.CPUUsage * 0.43 + rand(-1.4, 1.4), 36, 93);
      t.GPUTemp = clamp(47 + t.GPUUsage * 0.35 + rand(-1.4, 1.4), 43, 90);
      t.CPUClock = Math.round(rand(4200, 5300) / 50) * 50;
      t.GPUClock = Math.round(rand(1950, 2835) / 15) * 15;
      t.CPUPower = clamp(16 + t.CPUUsage * 1.05 + rand(-4, 5), 8, 160);
      t.GPUPower = clamp(45 + t.GPUUsage * 2.65 + rand(-10, 12), 30, 420);
      t.CPUFan = Math.round(clamp(620 + t.CPUTemp * 17 + rand(-60, 60), 500, 2400));
      t.GPUFan = Math.round(clamp(500 + t.GPUTemp * 20 + rand(-70, 70), 0, 2800));

      // фреймген требует дополнительной видеопамяти
      const fgVram = this.frameGen >= 2 ? 700 + this.frameGen * 220 : 0;
      t.VRAM = clamp(5900 + fgVram + rand(-240, 280), 3800, VRAM_TOTAL * 0.95);
      t.VRAMPct = (t.VRAM / VRAM_TOTAL) * 100;
      t.RAMUsage = clamp(t.RAMUsage + rand(-2.2, 2.4), 32, 72);
      t.RAMUsed = (t.RAMUsage / 100) * RAM_TOTAL;
      t.RAMFree = RAM_TOTAL - t.RAMUsed;

      const b1 = Math.random();
      t.DiskRead = b1 > 0.95 ? rand(140, 520) : b1 > 0.85 ? rand(12, 85) : rand(0, 5);
      const b2 = Math.random();
      t.DiskWrite = b2 > 0.94 ? rand(80, 310) : b2 > 0.87 ? rand(6, 55) : rand(0, 4);
      t.DiskActive = clamp((t.DiskRead + t.DiskWrite) / 5 + rand(0, 7), 0, 100);
      t.NetDown = Math.random() > 0.9 ? rand(500, 5400) : rand(10, 230);
      t.NetUp = Math.random() > 0.92 ? rand(200, 1700) : rand(4, 85);
    }

    this.targets.Frametime = 1000 / Math.max(this.targets.FPS, 1) + rand(-0.3, 0.4);

    for (const k of ALL) {
      const f = 1 - Math.exp(-dt / (TAU[k] ?? 500));
      this.values[k] += (this.targets[k] - this.values[k]) * f;
    }

    if (this.tPush >= PUSH_MS) {
      this.tPush %= PUSH_MS;
      this.push();
    }
  }

  private push(mul = 1) {
    for (const k of ALL) {
      const arr = this.history[k];
      const next = arr.length >= HISTORY_SIZE ? arr.slice(1) : arr.slice();
      next.push(Math.max(0, Math.round(this.values[k] * mul * 100) / 100));
      this.history[k] = next;
    }
  }

  snapshot(): SimSnapshot {
    return { values: { ...this.values }, history: this.history, fg: this.frameGen };
  }
}
