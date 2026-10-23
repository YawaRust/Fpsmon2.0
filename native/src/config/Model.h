#pragma once
#include "../common.h"

/*
 * Модель данных оверлея: реестр датчиков, блоки/элементы, темы,
 * hotkey-конфигурация. Полное зеркало веб-редактора и config.ini.
 */

enum class SensorId : int {
    FPS = 0, Frametime, FPSAvg, FPS1Low, FPSNative, FPSGen, FrameGenX, Latency,
    CPUUsage, CPUTemp, CPUClock, CPUPower, CPUFan,
    GPUUsage, GPUTemp, GPUClock, GPUPower, GPUFan, VRAM, VRAMPct,
    RAMUsage, RAMUsed, RAMFree,
    DiskRead, DiskWrite, DiskActive, NetDown, NetUp,
    COUNT
};

inline constexpr int SENSOR_COUNT = (int)SensorId::COUNT;
inline constexpr int HISTORY_SIZE = 120;

/* группа оборудования: 0 fps · 1 cpu · 2 gpu · 3 ram · 4 io */
struct SensorInfo {
    const char* key;     // имя в config.ini
    const char* name;    // имя по умолчанию
    const char* unit;
    int hw;              // группа (индекс в палитре темы)
    bool warnLow;        // true → срабатывает при значении НИЖЕ порога (FPS)
    int decimals;
    bool available;      // можно измерить нативно без vendor-SDK
    const char* ifUnavailable; // пояснение для скрытых датчиков
};

const SensorInfo& sensorInfo(SensorId id);
bool sensorByKey(const char* key, SensorId& out);

/* ---------- элементы и блоки ---------- */

struct ItemCfg {
    SensorId sensor;
    std::wstring label;
    bool showLabel = true;
    bool showUnit = true;
    bool sameLine = false;
    bool big = false;
    bool showGraph = false;
    float warn = 0.f, crit = 0.f;
    bool fgBreakdown = false;
};

struct BlockCfg {
    std::wstring title = L"BLOCK";
    bool showTitle = true;
    bool enabled = true;
    int hw = 1;
    std::vector<ItemCfg> items;
};

/* ---------- темы ---------- */

struct ThemeDef {
    const char* id;
    const char* display;
    const char* font;        // имя шрифта для DWrite
    bool bold;
    uint32_t bg, border, label, accent;
    float bgA;
    bool borderOn, shadow, separators, uppercase;
    int radius;
    int titleStyle;          // 0 plain · 1 chip · 2 bar
    float letterSpacing;
    int fontSize;
    int padding;
    uint32_t palette[5];     // цвет значений по группам hw
};

extern const ThemeDef THEMES[8];
extern const int THEME_COUNT;
int themeIndex(const std::string& id);

/* ---------- хоткеи ---------- */

enum HkAction : int {
    HK_ToggleOverlay = 0, HK_ToggleConfig, HK_CycleTheme,
    HK_CyclePosition, HK_ResetStats, HK_ToggleCapture, HK_COUNT
};

struct HotkeyCfg { UINT vk = 0; bool ctrl = false, shift = false, alt = false; };
UINT hkMods(const HotkeyCfg& h);        // MOD_* для RegisterHotKey
std::string hkLabel(const HotkeyCfg& h);

/* ---------- settings ---------- */

struct Settings {
    // позиция: 0 tl · 1 tr · 2 bl · 3 br · 4 custom
    int position = 0;
    int customX = 14, customY = 14;

    // оформление (кеш из темы)
    std::string theme = "neon";
    float fontSize = 14;
    bool fontBold = false;
    float opacity = 0.96f;
    float bgOpacity = 0.72f;
    int padding = 10, rowGap = 2, blockGap = 7, cornerRadius = 10;
    int direction = 0;       // 0 column · 1 row
    int titleStyle = 0;
    bool showSeparators = true, shadow = true, showBorder = true, uppercase = true;

    // опрос
    int updateIntervalMs = 1000;
    int fpsIntervalMs = 500;

    // окно
    bool clickThrough = false;
    bool alwaysOnTop = true;

    // захват сторонними программами (OBS): true → WDA_NONE
    bool captureAllow = true;

    // автозапуск
    bool autostart = false;
    bool autostartTask = false;   // false → реестр HKCU\Run · true → schtasks
    int  autostartDelay = 0;
    bool autostartMinimized = true;
    bool autostartApplied = false; // фактически записано в систему

    // фреймген (модуль per-process hook планируется в v0.1)
    bool fgDetect = true;

    HotkeyCfg hk[HK_COUNT];

    std::vector<BlockCfg> blocks;

    const ThemeDef& themeDef() const { return THEMES[themeIndex(theme)]; }

    void setDefaults();
    void applyTheme(const std::string& id);
    bool load();
    bool save() const;
    void sanitize();
};

std::wstring getAppDataPath();
std::string configPath();
std::string logPath();
