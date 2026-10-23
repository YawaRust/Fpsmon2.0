# Changelog

## v0.0.1 — 2026-10-23

### Исправления
- Merge pull request #2 from YawaRust/arena/83ccaa60-fpsmon2-0 ()
- fix: proper semver versioning from conventional commits ()
- fix: apply web settings in native panel and enable overlay dragging ()
- Merge pull request #1 from YawaRust/arena/ff1b7f56-fpsmon2-0 ()
- fix: define CPU power info struct for CallNtPowerInformation ()
- fix: make MSVC optimization flags Release-only ()
- fix: add prebuilt app.ico to avoid CMake HEX_DECODE failure ()
- fix: correct YAML indentation for embedded UI here-strings ()
- fix: repair CI embedding and native sensor/GPU handling ()

### Прочее
- Add files via upload ()
- Fix_2 ()


## v0.0.1 — 2026-10-23

### Исправления
- Merge pull request #1 from YawaRust/arena/ff1b7f56-fpsmon2-0 ()
- fix: define CPU power info struct for CallNtPowerInformation ()
- fix: make MSVC optimization flags Release-only ()
- fix: add prebuilt app.ico to avoid CMake HEX_DECODE failure ()
- fix: correct YAML indentation for embedded UI here-strings ()
- fix: repair CI embedding and native sensor/GPU handling ()

### Прочее
- Add files via upload ()
- Fix_2 ()


Все заметные изменения проекта. Версии идут по `0.0.X` — каждый фикс-релиз. Формат основан на [Keep a Changelog](https://keepachangelog.com/ru/1.1.0/),
версии — по [Semantic Versioning](https://semver.org/lang/ru/). Файл обновляется автоматически
воркфлоу `.github/workflows/release.yml` при каждом релизе.

## v0.0.2

### Исправления
- fix(ui): символ °C отображался как вопросик — исправлена кодировка UTF-8 (`\xC2\xB0`)
- fix(ui): шрифты Impact и Lucida Console не поддерживали кириллицу → заменены на Consolas и Segoe UI
- fix(drag): перетаскивание оверлея было тяжёлым — теперь двигает окно через `SetWindowPos` без полной перерисовки
- fix(drag): оверлей улетал за край экрана — клиппинг по виртуальному экрану (все мониторы)
- fix(click-through): переключение сквозных кликов не работало — добавлен `SWP_FRAMECHANGED` для обновления стилей окна
- fix(render): данные теперь перерисовываются каждый тик, без клика по оверлею
- fix(layout): конфиг-панель отделена от оверлея зазором 14 px, drag не перехватывает клики по панели
- fix(input): ПКМ по оверлею больше не открывает настройки
- fix(build): `wideToUtf8(std::wstring)` — добавлена перегрузка для `std::wstring`
- fix(unicode): все строки модели (`label`, `title`) хранятся как `std::wstring`, конфиг-панель на `wchar_t*`
- fix(unicode): темы `Cyber Magenta` и `RTSS Classic` переведены на шрифты с кириллицей (`Segoe UI`, `Consolas`)
- fix(tray): меню трея на `AppendMenuW` — кириллица отображается корректно
- fix(paths): конфиг и логи в `%LOCALAPPDATA%\YawaFPSHUD`
- feat(defaults): FPS и Frametime разделены на отдельные блоки по умолчанию

### Изменения
- chore: версия 0.0.2
- chore: в релиз идёт только `YawaFPSHUD.exe` (без ZIP и лишних файлов)

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
