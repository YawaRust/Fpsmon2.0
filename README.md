# YawaFPS HUD

[![CI](https://github.com/yawaside/yawafps-hud/actions/workflows/ci.yml/badge.svg)](https://github.com/yawaside/yawafps-hud/actions/workflows/ci.yml)
[![Release](https://github.com/yawaside/yawafps-hud/actions/workflows/release.yml/badge.svg)](https://github.com/yawaside/yawafps-hud/actions/workflows/release.yml)
![Size](https://img.shields.io/badge/exe-318%20КБ-00ff88)
![RAM](https://img.shields.io/badge/RAM-~5%20МБ-00d8ff)

Сверхлёгкий нативный оверлей мониторинга для Windows. Блоки по оборудованию,
настройка **прямо в игре**, поддержка генерации кадров (DLSS 3/4, FSR 3, XeSS-FG).
Чистый C++20 + WinAPI + Direct2D — без Qt, Electron и WebView.

## Возможности

| | |
|---|---|
| **Блоки по железу** | CPU, GPU, RAM, кадры, диск и сеть — отдельными группами |
| **Настройка в оверлее** | редактор открывается поверх игры, сворачиваться не нужно |
| **Свои имена** | любой показатель переименовывается, имя можно скрыть |
| **Гибкая раскладка** | порядок блоков и строк, объединение в одну строку, горизонтальная полоска |
| **8 готовых тем** | Neon Grid, RTSS Classic, Frosted Glass, Terminal, Cyber Magenta, Minimal Mono, Esport Bar, Retro CRT |
| **Frame Generation** | раздельно нативные и сгенерированные кадры, множитель и задержка |
| **Свои хоткеи** | шесть действий, назначаются в один клик |
| **Захват в OBS** | переключатель `WDA_NONE` / `WDA_EXCLUDEFROMCAPTURE` |
| **Автозапуск** | через `HKCU\...\Run` или планировщик задач, с задержкой |

## Ресурсы

| Метрика | Значение |
|---|---|
| ОЗУ процесса | 3–8 МБ |
| CPU (idle) | < 0.5 % |
| GPU | Direct2D, < 1 % |
| Размер EXE | ~318 КБ |
| Процессов | 1 |

## Горячие клавиши по умолчанию

| Комбинация | Действие |
|---|---|
| `Shift + F12` | показать / скрыть оверлей |
| `Shift + F11` | редактор настроек в игре |
| `Shift + F10` | следующая тема |
| `Shift + F9`  | следующий угол экрана |
| `Shift + F8`  | сбросить AVG / 1% LOW |
| `Shift + F7`  | переключить захват сторонними программами |

Все сочетания переназначаются во вкладке **КЛАВИШИ** встроенного редактора.

## Сборка

```bash
git clone https://github.com/yawaside/yawafps-hud.git
cd yawafps-hud
cmake -B build -S native -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
# готовый бинарник: build/YawaFPSHUD.exe
```

> Если собираете в PowerShell и передаёте версию вручную, обязательно
> закавычьте `-D`: `cmake ... "-DAPP_VERSION=$env:APPVER"` — иначе pwsh
> разобьёт `0.0.1` на отдельный аргумент.

Требуется Windows 10/11 x64, MSVC 2022 (C++20), CMake 3.20+, Windows SDK 10.0+.

## Структура репозитория

```
native/          нативное приложение C++20
  CMakeLists.txt     /O2 · /GL · /LTCG, версия из CI
  src/main.cpp       трей, RegisterHotKey, message loop
  src/config/        модель блоков, темы, INI
  src/core/          PDH + DXGI 1.4 + DWM сенсоры
  src/overlay/       layered окно, D2D рендер, in-game редактор
  resources/         version info, манифест PerMonitorV2
.github/workflows/   release.yml (автотег/changelog/exe) + ci.yml
docs(сайт)/          исходники демо-сайта (Vite + React)
```

## Релизы

Релизы полностью автоматические. При пуше в `main`:

1. версия считается по [Conventional Commits](https://www.conventionalcommits.org/ru/) —
   `feat:` → minor, `fix:`/`perf:` → patch, `!`/`BREAKING CHANGE` → major;
2. проект собирается на `windows-latest` (MSVC + Ninja) с проверкой бюджета размера;
3. генерируется changelog, сгруппированный по типам коммитов;
4. ставится тег `vX.Y.Z` и обновляется `CHANGELOG.md`;
5. публикуется GitHub Release с `.exe`, `.zip` и `SHA256SUMS.txt`.

Ручной выпуск: вкладка **Actions → Release → Run workflow** с выбором `patch`/`minor`/`major`.

## Конфигурация

Настройки хранятся в `config.ini` рядом с исполняемым файлом и полностью
редактируются из оверлея. Пример секций:

```ini
[Capture]
mode=allow
displayAffinity=WDA_NONE

[Autostart]
enabled=1
method=registry
delaySeconds=10
startMinimized=1

[Block_1]
title=CPU
hardware=cpu

[Block_1.Item_0]
sensor=CPUUsage
label=
showLabel=0
sameLine=0
```

## Лицензия

MIT © [yawaside](https://github.com/yawaside)
