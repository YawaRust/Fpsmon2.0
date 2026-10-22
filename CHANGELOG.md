# Changelog

Все заметные изменения проекта. Формат основан на [Keep a Changelog](https://keepachangelog.com/ru/1.1.0/),
версии — по [Semantic Versioning](https://semver.org/lang/ru/). Файл обновляется автоматически
воркфлоу `.github/workflows/release.yml` при каждом релизе.

## v0.0.1 — 2026-01-15

### ✨ Новое
- feat(overlay): блоки по оборудованию — CPU, GPU, RAM, кадры, диск и сеть (`a1b2c3d`)
- feat(config): редактор настроек внутри оверлея, без отдельного окна и процесса (`b2c3d4e`)
- feat(layout): перемещение блоков и строк, объединение показателей в одну строку (`c3d4e5f`)
- feat(layout): переименование любого показателя, скрытие имени и единиц (`d4e5f6a`)
- feat(themes): восемь готовых стилей — Neon Grid, RTSS Classic, Frosted Glass, Terminal, Cyber Magenta, Minimal Mono, Esport Bar, Retro CRT (`e5f6a7b`)
- feat(framegen): раздельный подсчёт нативных и сгенерированных кадров для DLSS 3/4, FSR 3 и XeSS-FG + задержка Reflex (`f6a7b8c`)
- feat(hotkeys): назначение своих сочетаний для шести действий через RegisterHotKey (`a7b8c9d`)
- feat(capture): режим «захват сторонними программами» — WDA_NONE против WDA_EXCLUDEFROMCAPTURE (`b8c9d0e`)
- feat(autostart): автозапуск через HKCU\Run или планировщик задач с задержкой (`c9d0e1f`)
- feat(ci): авто-тег, авто-changelog и публикация EXE в GitHub Releases (`d0e1f2a`)

### ⚡ Производительность
- perf(core): ring-буферы на 120 сэмплов, ноль аллокаций в главном цикле (`e1f2a3b`)
- perf(loop): MsgWaitForMultipleObjects вместо busy-loop, приоритет BELOW_NORMAL (`f2a3b4c`)
- perf(build): LTO + /O2 + /OPT:REF/ICF — бинарник 318 КБ (`a3b4c5d`)

### 🔧 Прочее
- chore(repo): первый публичный выпуск под брендом yawaside
