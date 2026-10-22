#include "Model.h"
#include "../utils/Logger.h"

/* ---------- реестр датчиков ---------- */

static const SensorInfo SENSORS[SENSOR_COUNT] = {
    /* key            name      unit      hw low dec avail  note */
    { "FPS",        "FPS",     "",     0, true,  0, true,  nullptr },
    { "Frametime",  "FT",      "ms",   0, false, 1, true,  nullptr },
    { "FPSAvg",     "AVG",     "",     0, true,  0, true,  nullptr },
    { "FPS1Low",    "1%",      "",     0, true,  0, true,  nullptr },
    { "FPSNative",  "NAT",     "",     0, true,  0, true,  nullptr },
    { "FPSGen",     "GEN",     "",     0, false, 0, false, "нужен present-hook (v0.1)" },
    { "FrameGenX",  "FG",      "x",    0, false, 0, false, "нужен present-hook (v0.1)" },
    { "Latency",    "LAT",     "ms",   0, false, 0, false, "нужен NVAPI/PCL (v0.1)" },

    { "CPUUsage",   "LOAD",    "%",    1, false, 0, true,  nullptr },
    { "CPUTemp",    "TEMP",    "\xB0""C",  1, false, 0, true,  nullptr }, // через PDH Thermal Zone, если есть
    { "CPUClock",   "CLK",     "MHz",  1, false, 0, true,  nullptr },
    { "CPUPower",   "PWR",     "W",    1, false, 0, false, "нужен Intel RAPL / AMD SMU" },
    { "CPUFan",     "FAN",     "RPM",  1, false, 0, false, "нужен драйвер EC" },

    { "GPUUsage",   "LOAD",    "%",    2, false, 0, true,  nullptr },
    { "GPUTemp",    "TEMP",    "\xB0""C",  2, false, 0, true,  nullptr },
    { "GPUClock",   "CLK",     "MHz",  2, false, 0, false, "нужен NVAPI / ADL" },
    { "GPUPower",   "PWR",     "W",    2, false, 0, false, "нужен NVAPI / ADL" },
    { "GPUFan",     "FAN",     "RPM",  2, false, 0, false, "нужен NVAPI / ADL" },
    { "VRAM",       "VRAM",    "MB",   2, false, 0, true,  nullptr },
    { "VRAMPct",    "VRAM",    "%",    2, false, 0, true,  nullptr },

    { "RAMUsage",   "LOAD",    "%",    3, false, 0, true,  nullptr },
    { "RAMUsed",    "USED",    "MB",   3, false, 0, true,  nullptr },
    { "RAMFree",    "FREE",    "MB",   3, false, 0, true,  nullptr },

    { "DiskRead",   "READ",    "MB/s", 4, false, 1, true,  nullptr },
    { "DiskWrite",  "WRITE",   "MB/s", 4, false, 1, true,  nullptr },
    { "DiskActive", "DISK",    "%",    4, false, 0, true,  nullptr },
    { "NetDown",    "DOWN",    "KB/s", 4, false, 0, true,  nullptr },
    { "NetUp",      "UP",      "KB/s", 4, false, 0, true,  nullptr },
};

const SensorInfo& sensorInfo(SensorId id) { return SENSORS[(int)id]; }

bool sensorByKey(const char* key, SensorId& out) {
    for (int i = 0; i < SENSOR_COUNT; i++)
        if (_stricmp(SENSORS[i].key, key) == 0) { out = (SensorId)i; return true; }
    return false;
}

/* ---------- темы ---------- */

const ThemeDef THEMES[8] = {
    // id          display          font           bold  bg        border    label     accent    bgA   brd  shdw  sep   upper radius title ls     fs   pad   palette fps,cpu,gpu,ram,io
    { "neon",     "Neon Grid",     "Consolas",    false,0x05070B, 0x94A3B8, 0xCBD5E1, 0x00FF88, .72f, true, true, true, true, 10, 0, .06f, 14, 10, {0x22FF66,0x4DB3FF,0xB9A7FF,0xFFCC4D,0x6EF3D6} },
    { "rtss",     "RTSS Classic",  "Lucida Console", true,0x000000, 0x000000, 0x9FE8B4, 0x44FF77, .0f, false,false,false,true,  0, 0, .02f, 16, 4,  {0x44FF77,0x44FF77,0x44FF77,0x44FF77,0x44FF77} },
    { "glass",    "Frosted Glass", "Segoe UI",    true, 0xF3F6FB, 0xFFFFFF, 0xDBE6F5, 0xFFFFFF, .16f, true, true, false,false, 14, 1, .01f, 14, 12, {0xFFFFFF,0xBCD9FF,0xE2C9FF,0xFFE6AB,0xB6FFF0} },
    { "terminal", "Terminal",      "Consolas",    false,0x000000, 0xFFB03A, 0x9A6B22, 0xFFB03A, .86f, true, false,true, true,  0, 2, .12f, 13, 8,  {0xFFB03A,0xFFC76B,0xFFA021,0xE09020,0xD98020} },
    { "cyber",    "Cyber Magenta", "Impact",      true, 0x120418, 0xFF2BD1, 0xC58AE0, 0xFF2BD1, .78f, true, true, true, true,  4, 2, .18f, 13, 11, {0xFF2BD1,0x00E5FF,0xB47AFF,0xFFD23F,0x6EF3D6} },
    { "minimal",  "Minimal Mono",  "Segoe UI",    true, 0x000000, 0xFFFFFF, 0x8F98A6, 0xFFFFFF, .28f, false,false,false,false, 8, 0, 0.f,  14, 8,  {0xFFFFFF,0xE2E8F0,0xE2E8F0,0xE2E8F0,0xE2E8F0} },
    { "esport",   "Esport Bar",    "Consolas",    true, 0x070B12, 0x1F2B3D, 0x64748B, 0x38BDF8, .9f,  true, true, true, true,  6, 1, .08f, 13, 7,  {0x38BDF8,0xF472B6,0xA78BFA,0xFACC15,0x4ADE80} },
    { "crt",      "Retro CRT",     "Courier New", false,0x021206, 0x2DFF6A, 0x1F8F3F, 0x2DFF6A, .82f, true, false,true, true,  3, 2, .14f, 15, 10, {0x2DFF6A,0x55FF88,0x88FFAA,0xAAFFBB,0x2DFF6A} },
};
const int THEME_COUNT = 8;

int themeIndex(const std::string& id) {
    for (int i = 0; i < THEME_COUNT; i++)
        if (_stricmp(THEMES[i].id, id.c_str()) == 0) return i;
    return 0;
}

/* ---------- хоткеи ---------- */

UINT hkMods(const HotkeyCfg& h) {
    UINT m = 0;
    if (h.alt)   m |= MOD_ALT;
    if (h.ctrl)  m |= MOD_CONTROL;
    if (h.shift) m |= MOD_SHIFT;
    return m;
}

std::string hkLabel(const HotkeyCfg& h) {
    if (!h.vk) return "-";
    std::string s;
    if (h.ctrl)  s += "Ctrl+";
    if (h.shift) s += "Shift+";
    if (h.alt)   s += "Alt+";
    char name[32] = {};
    if (h.vk >= VK_F1 && h.vk <= VK_F24) sprintf_s(name, "F%d", (int)h.vk - VK_F1 + 1);
    else if (h.vk >= 'A' && h.vk <= 'Z') sprintf_s(name, "%c", (char)h.vk);
    else if (h.vk >= '0' && h.vk <= '9') sprintf_s(name, "%c", (char)h.vk);
    else {
        UINT scan = MapVirtualKeyA(h.vk, MAPVK_VK_TO_VSC);
        GetKeyNameTextA((LONG)(scan << 16), name, (int)sizeof(name));
        if (!name[0]) sprintf_s(name, "VK%02X", h.vk);
    }
    return s + name;
}

/* ---------- путь к config.ini ---------- */

const char* configPath() {
    static char path[MAX_PATH] = {};
    if (!path[0]) {
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        char* slash = strrchr(path, '\\');
        if (slash) strcpy_s(slash + 1, size_t(path + MAX_PATH - slash - 1), "config.ini");
    }
    return path;
}

/* ---------- defaults ---------- */

static ItemCfg mk(SensorId s, const char* label, bool showLabel = true,
                  bool sameLine = false, bool graph = false, bool big = false) {
    ItemCfg it;
    it.sensor = s;
    it.label = label;
    it.showLabel = showLabel;
    it.sameLine = sameLine;
    it.showGraph = graph;
    it.big = big;
    const SensorInfo& info = sensorInfo(s);
    if (info.warnLow) { it.warn = 45; it.crit = 30; }
    else if (s == SensorId::CPUUsage) { it.warn = 80; it.crit = 95; }
    else if (s == SensorId::CPUTemp)  { it.warn = 75; it.crit = 90; }
    else if (s == SensorId::GPUTemp)  { it.warn = 80; it.crit = 90; }
    else if (s == SensorId::Frametime){ it.warn = 20; it.crit = 33; }
    else if (s == SensorId::RAMUsage) { it.warn = 85; it.crit = 95; }
    else if (s == SensorId::VRAMPct)  { it.warn = 85; it.crit = 95; }
    return it;
}

void Settings::setDefaults() {
    blocks.clear();

    BlockCfg fps; fps.hw = 0; fps.title.clear(); fps.showTitle = false;
    fps.items.push_back(mk(SensorId::FPS, "", false, false, true, true));
    fps.items.push_back(mk(SensorId::FPS1Low, "1%", true, true));
    fps.items.push_back(mk(SensorId::Frametime, "FT"));
    blocks.push_back(fps);

    BlockCfg cpu; cpu.hw = 1; cpu.title = "CPU";
    cpu.items.push_back(mk(SensorId::CPUUsage, "", false, false, true));
    cpu.items.push_back(mk(SensorId::CPUTemp, "", false, true));
    cpu.items.push_back(mk(SensorId::CPUClock, "CLK"));
    blocks.push_back(cpu);

    BlockCfg gpu; gpu.hw = 2; gpu.title = "GPU";
    gpu.items.push_back(mk(SensorId::GPUUsage, "", false, false, true));
    gpu.items.push_back(mk(SensorId::GPUTemp, "", false, true));
    gpu.items.push_back(mk(SensorId::VRAM, "VRAM"));
    blocks.push_back(gpu);

    BlockCfg ram; ram.hw = 3; ram.title = "RAM";
    ram.items.push_back(mk(SensorId::RAMUsage, "", false));
    ram.items.push_back(mk(SensorId::RAMUsed, "", false, true));
    blocks.push_back(ram);

    hk[HK_ToggleOverlay] = { VK_F12, false, true, false };
    hk[HK_ToggleConfig]  = { VK_F11, false, true, false };
    hk[HK_CycleTheme]    = { VK_F10, false, true, false };
    hk[HK_CyclePosition] = { VK_F9,  false, true, false };
    hk[HK_ResetStats]    = { VK_F8,  false, true, false };
    hk[HK_ToggleCapture] = { VK_F7,  false, true, false };

    applyTheme(theme);
}

void Settings::applyTheme(const std::string& id) {
    theme = id;
    const ThemeDef& t = themeDef();
    fontSize = (float)t.fontSize;
    fontBold = t.bold;
    bgOpacity = t.bgA;
    cornerRadius = t.radius;
    padding = t.padding;
    titleStyle = t.titleStyle;
    showSeparators = t.separators;
    shadow = t.shadow;
    showBorder = t.borderOn;
    uppercase = t.uppercase;
}

void Settings::sanitize() {
    clampf(fontSize, 9.f, 28.f);
    clampf(opacity, 0.2f, 1.f);
    clampf(bgOpacity, 0.f, 1.f);
    if (updateIntervalMs < 100) updateIntervalMs = 100;
    if (fpsIntervalMs < 50) fpsIntervalMs = 50;
    position = position < 0 ? 0 : (position > 4 ? 4 : position);
}

/* ---------- INI ---------- */

static bool iniBool(const char* v) { return v && (v[0] == '1' || _stricmp(v, "true") == 0); }

bool Settings::load() {
    setDefaults();

    const char* path = configPath();
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
        LOG("config.ini не найден — создаю по умолчанию");
        save();
        return false;
    }

    // ── общие секции ──
    position         = GetPrivateProfileIntA("General", "position", position, path);
    customX          = GetPrivateProfileIntA("General", "customX", customX, path);
    customY          = GetPrivateProfileIntA("General", "customY", customY, path);
    direction        = GetPrivateProfileIntA("General", "direction", direction, path);
    updateIntervalMs = GetPrivateProfileIntA("General", "updateInterval", updateIntervalMs, path);
    fpsIntervalMs    = GetPrivateProfileIntA("General", "fpsUpdateInterval", fpsIntervalMs, path);
    alwaysOnTop      = GetPrivateProfileIntA("General", "alwaysOnTop", alwaysOnTop ? 1 : 0, path) != 0;
    clickThrough     = GetPrivateProfileIntA("General", "clickThrough", clickThrough ? 1 : 0, path) != 0;
    fgDetect         = GetPrivateProfileIntA("General", "frameGenDetect", fgDetect ? 1 : 0, path) != 0;

    char buf[512];
    if (GetPrivateProfileStringA("Capture", "mode", "", buf, sizeof(buf), path) > 0)
        captureAllow = _stricmp(buf, "allow") == 0;

    autostart          = GetPrivateProfileIntA("Autostart", "enabled", autostart ? 1 : 0, path) != 0;
    if (GetPrivateProfileStringA("Autostart", "method", "registry", buf, sizeof(buf), path) > 0)
        autostartTask = _stricmp(buf, "task") == 0;
    autostartDelay     = GetPrivateProfileIntA("Autostart", "delaySeconds", autostartDelay, path);
    autostartMinimized = GetPrivateProfileIntA("Autostart", "startMinimized", autostartMinimized ? 1 : 0, path) != 0;
    autostartApplied   = GetPrivateProfileIntA("Autostart", "applied", 0, path) != 0;

    if (GetPrivateProfileStringA("Appearance", "theme", theme.c_str(), buf, sizeof(buf), path) > 0)
        applyTheme(buf);
    fontSize = (float)GetPrivateProfileIntA("Appearance", "fontSize", (int)fontSize, path);
    fontBold = GetPrivateProfileIntA("Appearance", "fontBold", fontBold ? 1 : 0, path) != 0;
    padding  = GetPrivateProfileIntA("Appearance", "padding", padding, path);
    rowGap   = GetPrivateProfileIntA("Appearance", "rowGap", rowGap, path);
    blockGap = GetPrivateProfileIntA("Appearance", "blockGap", blockGap, path);
    titleStyle = GetPrivateProfileIntA("Appearance", "titleStyle", titleStyle, path);
    uppercase  = GetPrivateProfileIntA("Appearance", "uppercase", uppercase ? 1 : 0, path) != 0;
    showSeparators = GetPrivateProfileIntA("Appearance", "separators", showSeparators ? 1 : 0, path) != 0;

    // ── хоткеи ──
    static const char* hkNames[HK_COUNT] = {
        "toggleOverlay", "toggleConfig", "cycleTheme", "cyclePosition", "resetStats", "toggleCapture"
    };
    for (int i = 0; i < HK_COUNT; i++) {
        if (GetPrivateProfileStringA("Hotkeys", hkNames[i], "", buf, sizeof(buf), path) > 0) {
            UINT vk = 0, mods = 0;
            if (sscanf_s(buf, "%u,%u", &vk, &mods) == 2) {
                hk[i].vk = vk;
                hk[i].alt = (mods & MOD_ALT) != 0;
                hk[i].ctrl = (mods & MOD_CONTROL) != 0;
                hk[i].shift = (mods & MOD_SHIFT) != 0;
            }
        }
    }

    // ── блоки ──
    std::vector<BlockCfg> loaded;
    for (int bi = 0; bi < 32; bi++) {
        char sect[32]; sprintf_s(sect, "Block_%d", bi);
        if (GetPrivateProfileStringA(sect, "hardware", "", buf, sizeof(buf), path) == 0)
            break;

        BlockCfg b;
        static const int hwIdByName[] = { 0, 1, 2, 3, 4 };
        static const char* hwNames[] = { "fps", "cpu", "gpu", "ram", "io" };
        b.hw = 1;
        for (int h = 0; h < 5; h++) if (_stricmp(buf, hwNames[h]) == 0) { b.hw = hwIdByName[h]; break; }

        if (GetPrivateProfileStringA(sect, "title", "", buf, sizeof(buf), path) > 0) b.title = buf; else b.title.clear();
        b.showTitle = GetPrivateProfileIntA(sect, "showTitle", 1, path) != 0;
        b.enabled   = GetPrivateProfileIntA(sect, "enabled", 1, path) != 0;
        int count   = GetPrivateProfileIntA(sect, "itemCount", 0, path);

        for (int ii = 0; ii < count && ii < 64; ii++) {
            char isect[40]; sprintf_s(isect, "Block_%d.Item_%d", bi, ii);
            if (GetPrivateProfileStringA(isect, "sensor", "", buf, sizeof(buf), path) == 0) continue;
            SensorId sid;
            if (!sensorByKey(buf, sid)) continue;

            ItemCfg it = mk(sid, sensorInfo(sid).name);
            if (GetPrivateProfileStringA(isect, "label", "", buf, sizeof(buf), path) > 0) it.label = buf;
            it.showLabel = GetPrivateProfileIntA(isect, "showLabel", 1, path) != 0;
            it.showUnit  = GetPrivateProfileIntA(isect, "showUnit", 1, path) != 0;
            it.sameLine  = GetPrivateProfileIntA(isect, "sameLine", 0, path) != 0;
            it.big       = GetPrivateProfileIntA(isect, "bigFont", 0, path) != 0;
            it.showGraph = GetPrivateProfileIntA(isect, "showGraph", 0, path) != 0;
            it.warn = (float)GetPrivateProfileIntA(isect, "warningThreshold", (int)it.warn, path);
            it.crit = (float)GetPrivateProfileIntA(isect, "criticalThreshold", (int)it.crit, path);
            b.items.push_back(it);
        }
        loaded.push_back(b);
    }
    if (!loaded.empty()) blocks = loaded;

    sanitize();
    LOG("конфигурация загружена");
    return true;
}

bool Settings::save() const {
    const char* path = configPath();
    FILE* f = nullptr;
    fopen_s(&f, path, "w");
    if (!f) { LOG("не удалось записать config.ini"); return false; }

    const auto B = [](bool v) { return v ? "1" : "0"; };

    fprintf(f, "; YawaFPS HUD %s\n[General]\n", APP_VERSION);
    fprintf(f, "version=%s\nposition=%d\ncustomX=%d\ncustomY=%d\n",
        APP_VERSION, position, customX, customY);
    fprintf(f, "direction=%d\nupdateInterval=%d\nfpsUpdateInterval=%d\n",
        direction, updateIntervalMs, fpsIntervalMs);
    fprintf(f, "alwaysOnTop=%s\nclickThrough=%s\nframeGenDetect=%s\n\n",
        B(alwaysOnTop), B(clickThrough), B(fgDetect));

    fprintf(f, "[Capture]\nmode=%s\ndisplayAffinity=%s\n\n",
        captureAllow ? "allow" : "exclude",
        captureAllow ? "WDA_NONE" : "WDA_EXCLUDEFROMCAPTURE");

    fprintf(f, "[Autostart]\nenabled=%s\nmethod=%s\ndelaySeconds=%d\nstartMinimized=%s\napplied=%s\n\n",
        B(autostart), autostartTask ? "task" : "registry",
        autostartDelay, B(autostartMinimized), B(autostartApplied));

    fprintf(f, "[Appearance]\ntheme=%s\nfontSize=%d\nfontBold=%s\nopacity=%.2f\n",
        theme.c_str(), (int)fontSize, B(fontBold), opacity);
    fprintf(f, "padding=%d\nrowGap=%d\nblockGap=%d\ntitleStyle=%d\nuppercase=%s\nseparators=%s\n\n",
        padding, rowGap, blockGap, titleStyle, B(uppercase), B(showSeparators));

    static const char* hkNames[HK_COUNT] = {
        "toggleOverlay", "toggleConfig", "cycleTheme", "cyclePosition", "resetStats", "toggleCapture"
    };
    fprintf(f, "[Hotkeys]\n");
    for (int i = 0; i < HK_COUNT; i++) {
        std::string l = hkLabel(hk[i]);
        fprintf(f, "%s=%u,%u   ; %s\n", hkNames[i], hk[i].vk, hkMods(hk[i]), l.c_str());
    }
    fprintf(f, "\n");

    for (size_t bi = 0; bi < blocks.size(); bi++) {
        const BlockCfg& b = blocks[bi];
        static const char* hwNames[] = { "fps", "cpu", "gpu", "ram", "io" };
        fprintf(f, "[Block_%d]\ntitle=%s\nshowTitle=%s\nenabled=%s\nhardware=%s\nitemCount=%d\n\n",
            (int)bi, b.title.c_str(), B(b.showTitle), B(b.enabled),
            hwNames[b.hw < 0 ? 0 : (b.hw > 4 ? 4 : b.hw)], (int)b.items.size());

        for (size_t ii = 0; ii < b.items.size(); ii++) {
            const ItemCfg& it = b.items[ii];
            const SensorInfo& info = sensorInfo(it.sensor);
            fprintf(f, "[Block_%d.Item_%d]\n; %s (%s)\n", (int)bi, (int)ii, info.name, info.unit[0] ? info.unit : "-");
            fprintf(f, "sensor=%s\nlabel=%s\nshowLabel=%s\nshowUnit=%s\nsameLine=%s\nbigFont=%s\nshowGraph=%s\n",
                info.key, it.label.c_str(), B(it.showLabel), B(it.showUnit),
                B(it.sameLine), B(it.big), B(it.showGraph));
            fprintf(f, "warningThreshold=%.0f\ncriticalThreshold=%.0f\n\n", it.warn, it.crit);
        }
    }

    fclose(f);
    LOG("конфигурация сохранена");
    return true;
}
