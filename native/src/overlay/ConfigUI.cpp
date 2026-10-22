#include "Overlay.h"
#include <cstdlib>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

/*
 * Immediate-mode интерфейс настроек внутри того же layered-окна.
 * Рисуется тем же D2D-контекстом, что и оверлей — без второго окна
 * и GUI-процесса. Состояние x/y/mouse-down живёт между вызовами.
 */

static int   uiMouseX = 0, uiMouseY = 0;
static bool  uiDown = false, uiWasDown = false;
static int   uiScroll = 0;
static int   uiHotkeyListening = -1;   // индекс HkAction, -1 = нет
static UINT  uiHkVK = 0;
static bool  uiHkShift = false, uiHkCtrl = false, uiHkAlt = false;

static RECT r(float x, float y, float w, float h) {
    RECT rc{ (LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h) };
    return rc;
}
static bool hit(float x, float y, float w, float h) {
    return uiMouseX >= x && uiMouseX < x + w && uiMouseY >= y && uiMouseY < y + h;
}

// простой фон
static void fillRect(Overlay& o, float x, float y, float w, float h, uint32_t rgb, float a, float rad = 5.f) {
    auto* rt = o.rt();
    ID2D1SolidColorBrush* b = nullptr;
    rt->CreateSolidColorBrush(toColorF(rgb, a), &b);
    D2D1_ROUNDED_RECT rr;
    rr.rect = D2D1::RectF(x, y, x + w, y + h);
    rr.radiusX = rad; rr.radiusY = rad;
    rt->FillRoundedRectangle(&rr, b);
    b->Release();
}
static void strokeRect(Overlay& o, float x, float y, float w, float h, uint32_t rgb, float a, float rad = 5.f, float sw = 1.f) {
    auto* rt = o.rt();
    ID2D1SolidColorBrush* b = nullptr;
    rt->CreateSolidColorBrush(toColorF(rgb, a), &b);
    D2D1_ROUNDED_RECT rr;
    rr.rect = D2D1::RectF(x, y, x + w, y + h);
    rr.radiusX = rad; rr.radiusY = rad;
    rt->DrawRoundedRectangle(&rr, b, sw);
    b->Release();
}
/* стиль — из темы настроек */
struct Ui {
    uint32_t bg, panel, border, accent, fg, dim;
} static g_ui;

static void applyUiColors(const ThemeDef& t) {
    g_ui.bg = t.bg; g_ui.panel = 0x0E1118;
    g_ui.border = t.border; g_ui.accent = t.accent;
    g_ui.fg = 0xE8EEF5; g_ui.dim = 0x7B8499;
}

/* маленький текст в верхнем регистре */
static void uiText(Overlay& o, float x, float y, const char* s, uint32_t c, float sz = 9.5f, bool bold = false, float a = 1.f) {
    std::string t = s;
    for (auto& ch : t) ch = (char)toupper((unsigned char)ch);
    o.text(x, y, t.c_str(), c, sz, bold, a);
}

/* ---------- элементы управления ---------- */

static bool button(Overlay& o, int id, float x, float y, float w, float h, const char* label, bool active = false) {
    bool hover = hit(x, y, w, h);
    uint32_t bgc = active ? g_ui.accent : (hover ? 0x1C2430 : 0x0E1118);
    float a = active ? 0.22f : 1.f;
    fillRect(o, x, y, w, h, bgc, a, 4.f);
    strokeRect(o, x + 0.5f, y + 0.5f, w - 1, h - 1, active ? g_ui.accent : g_ui.border, active ? 0.8f : 0.3f, 4.f, 1.f);
    uint32_t tc = active ? g_ui.accent : g_ui.fg;
    float tw = o.textW(label, 9.5f, true);
    uiText(o, x + (w - tw) / 2, y + h / 2 - 5, label, tc, 9.5f, true);
    if (hover && uiDown && !uiWasDown) { /* clicked */ (void)id; }
    return hover && !uiDown && uiWasDown; // отпускание по области
}

static bool toggleRow(Overlay& o, float x, float y, float w, const char* label, bool& v, const char* hint = nullptr) {
    float h = 18.f;
    float tx = x + w - 30;
    uiText(o, x + 2, y + 3, label, v ? g_ui.fg : g_ui.dim, 10.f, false, 0.95f);
    // переключатель
    fillRect(o, tx, y + 1, 26, 14, v ? g_ui.accent : 0x2A3342, v ? 0.9f : 0.9f, 7.f);
    fillRect(o, tx + (v ? 13 : 1) + 1, y + 2, 12, 12, v ? 0x05070B : 0xB0B8C4, 1.f, 6.f);
    if (hint) {
        float hw = o.textW(hint, 8.f, false);
        uiText(o, x + 2, y + h, hint, g_ui.dim, 8.f, false, 0.8f);
        (void)hw;
        return hit(tx, y, 26, 14) && uiDown && !uiWasDown;
    }
    if (hit(tx, y, 26, 14) && !uiDown && uiWasDown) { v = !v; return true; }
    return false;
}

static bool sliderRow(Overlay& o, float x, float y, float w, const char* label, float& v, float mn, float mx, const char* fmt = "%.0f") {
    float labelW = 120.f;
    uiText(o, x + 2, y + 3, label, g_ui.dim, 9.5f);
    char vb[24]; sprintf_s(vb, fmt, v);
    float vw = o.textW(vb, 9.5f, true);
    uiText(o, x + w - vw, y + 3, vb, g_ui.accent, 9.5f, true);
    // дорожка
    float tx = x + 2, ty = y + 13, tw2 = w - 4;
    fillRect(o, tx, ty, tw2, 4, 0x1F2634, 1.f, 2.f);
    float p = (v - mn) / (mx - mn); if (p < 0) p = 0; if (p > 1) p = 1;
    fillRect(o, tx, ty, tw2 * p, 4, g_ui.accent, 0.9f, 2.f);
    // бегунок
    float knobX = tx + tw2 * p;
    fillRect(o, knobX - 3, ty - 3, 6, 10, g_ui.accent, 1.f, 3.f);
    (void)labelW;
    if ((hit(tx, ty - 4, tw2, 16) && uiDown) || (hit(knobX - 6, ty - 4, 12, 16) && uiDown)) {
        float np = ((float)uiMouseX - tx) / tw2;
        if (np < 0) np = 0; if (np > 1) np = 1;
        v = mn + np * (mx - mn);
        return true;
    }
    return false;
}

/* ---------- ввод ---------- */

static void uiProcessInput(Overlay& o) {
    POINT pt; GetCursorPos(&pt);
    RECT wr; GetWindowRect(o.hwnd(), &wr);
    uiMouseX = pt.x - wr.left;
    uiMouseY = pt.y - wr.top;
    uiDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

static void uiEndFrame() {
    uiWasDown = uiDown;
}

/* колёсико мыши */
static void addScroll(int delta) {
    uiScroll += delta > 0 ? -22 : 22;
    if (uiScroll < 0) uiScroll = 0;
}

/* ---------- захват хоткеев ---------- */

static bool uiCaptureHotkey(HotkeyCfg& hk) {
    if (uiHotkeyListening < 0) return false;
    short ks[4] = { (short)GetAsyncKeyState(VK_SHIFT), (short)GetAsyncKeyState(VK_CONTROL),
                    (short)GetAsyncKeyState(VK_MENU), 0 };
    // ждём отпускания
    for (int k = VK_F1; k <= VK_F24; k++) {
        if (GetAsyncKeyState(k) & 1) {
            uiHkVK = (UINT)k;
            uiHkShift = (ks[0] & 0x8000) != 0;
            uiHkCtrl = (ks[1] & 0x8000) != 0;
            uiHkAlt = (ks[2] & 0x8000) != 0;
            hk.vk = uiHkVK; hk.shift = uiHkShift; hk.ctrl = uiHkCtrl; hk.alt = uiHkAlt;
            uiHotkeyListening = -1;
            return true;
        }
    }
    for (int k = 'A'; k <= 'Z'; k++) {
        if (GetAsyncKeyState(k) & 1) {
            uiHkVK = (UINT)k;
            uiHkShift = (ks[0] & 0x8000) != 0;
            uiHkCtrl = (ks[1] & 0x8000) != 0;
            uiHkAlt = (ks[2] & 0x8000) != 0;
            hk.vk = uiHkVK;
            hk.shift = uiHkShift; hk.ctrl = uiHkCtrl; hk.alt = uiHkAlt;
            uiHotkeyListening = -1;
            return true;
        }
    }
    return false;
}

/* ---------- перерисовка UI ---------- */

void drawConfigUI(Overlay& o) {
    Settings& s = o.settings();
    const ThemeDef& th = s.themeDef();
    applyUiColors(th);
    uiProcessInput(o);

    // панель справа от оверлея (внутри того же окна)
    float panelW = 304.f;
    float panelX = (float)o.m_cfgPanelX;
    float panelH = (float)o.m_h;
    float px = panelX + 4;
    float py = 4;
    float pw = panelW - 8;
    float ph = panelH - 8;

    // сама панель
    fillRect(o, px, py, pw, ph, g_ui.panel, 0.96f, 6.f);
    strokeRect(o, px + 0.5f, py + 0.5f, pw - 1, ph - 1, g_ui.accent, 0.4f, 6.f, 1.f);

    // заголовок с крестиком
    uiText(o, px + 10, py + 8, "IN-GAME CONFIG", g_ui.accent, 10.f, true);
    float v = o.textW("v" APP_VERSION, 8.f, false);
    uiText(o, px + pw - v - 26, py + 9, "v" APP_VERSION, g_ui.dim, 8.f);
    if (button(o, 998, px + pw - 22, py + 6, 16, 16, "X")) o.toggleConfig();

    // табы
    static const char* tabs[] = { "BLOCKS", "THEMES", "STYLE", "KEYS", "SYSTEM" };
    constexpr int NTABS = 5;
    if (o.m_cfgTab < 0 || o.m_cfgTab >= NTABS) o.m_cfgTab = 0;
    float tx = px + 8, tw = (pw - 16) / NTABS;
    for (int i = 0; i < NTABS; i++) {
        if (button(o, 10 + i, tx + i * tw, py + 28, tw - 2, 20, tabs[i], o.m_cfgTab == i))
            o.m_cfgTab = i;
    }

    // колёсико скролла
    if (GetAsyncKeyState(VK_MBUTTON) || false) {}
    short wheel = (short)HIWORD(0); // обычное колесо прилетает как WM_MOUSEWHEEL — обрабатываем в onMsg; здесь fallback на стрелки
    if (GetAsyncKeyState(VK_DOWN) & 1) addScroll(-40);
    if (GetAsyncKeyState(VK_UP) & 1) addScroll(40);
    (void)wheel;

    float cy = py + 56 + uiScroll;
    float cw = pw - 12;
    int idSeq = 100;

    /* ==================== БЛОКИ ==================== */
    if (o.m_cfgTab == 0) {
        for (size_t bi = 0; bi < s.blocks.size(); bi++) {
            BlockCfg& b = s.blocks[bi];
            float bh = 58.f;
            fillRect(o, px + 8, cy, cw, bh, 0x0B0F15, 1.f, 6.f);
            strokeRect(o, px + 8.5f, cy + 0.5f, cw - 1, bh - 1, b.enabled ? g_ui.accent : g_ui.border, b.enabled ? 0.35f : 0.15f, 6.f, 1.f);

            // стрелка вверх/вниз
            bool up = (bi > 0), dn = (bi + 1 < s.blocks.size());
            if (up && button(o, idSeq++, px + cw - 20, cy + 4, 14, 14, "^")) {
                std::swap(s.blocks[bi], s.blocks[bi - 1]); o.markDirty();
            }
            if (dn && button(o, idSeq++, px + cw - 20, cy + 20, 14, 14, "v")) {
                std::swap(s.blocks[bi], s.blocks[bi + 1]); o.markDirty();
            }

            // заголовок
            char head[64];
            strcpy_s(head, b.title.empty() ? "<без имени>" : b.title.c_str());
            uiText(o, px + 12, cy + 6, head, b.enabled ? g_ui.fg : g_ui.dim, 10.5f, true, b.enabled ? 1.f : 0.5f);
            char hwS[16];
            static const char* hwNames[] = { "FPS", "CPU", "GPU", "RAM", "I/O" };
            sprintf_s(hwS, "%s", hwNames[b.hw < 0 ? 0 : (b.hw > 4 ? 4 : b.hw)]);
            float hwW = o.textW(hwS, 8.f, true);
            fillRect(o, px + cw - hwW - 32, cy + 6, hwW + 10, 13, g_ui.accent, 0.15f, 3.f);
            uiText(o, px + cw - hwW - 27, cy + 8, hwS, g_ui.accent, 8.f, true);

            // тумблер enabled
            if (toggleRow(o, px + 12, cy + 24, cw - 24, "показывать", b.enabled))
                o.markDirty();

            // элементы с кнопками up/down, sameLine, удалить
            float iy = cy + 44;
            for (size_t ii = 0; ii < b.items.size(); ii++) {
                ItemCfg& it = b.items[ii];
                fillRect(o, px + 16, iy, cw - 8, 20, 0x10151D, 1.f, 4.f);
                const SensorInfo& si = sensorInfo(it.sensor);
                char rowBuf[128];
                const char* lbl = it.showLabel && !it.label.empty() ? it.label.c_str() : si.name;
                sprintf_s(rowBuf, "%s%s  %s", it.sameLine ? "- " : "  ", lbl, si.unit);
                uint32_t rc2 = it.showLabel || true ? g_ui.fg : g_ui.dim;
                uiText(o, px + 22, iy + 5, rowBuf, it.showGraph ? g_ui.accent : rc2, 9.5f, false, 0.95f);

                // текущее значение цветом из палитры темы
                char vb[32];
                const float val = o.monitor().value(it.sensor);
                if (val >= 10000.f) sprintf_s(vb, "%.0f", val);
                else sprintf_s(vb, "%.*f", si.decimals, val);
                const float vw2 = o.textW(vb, 9.5f, true);
                o.text(px + cw - vw2 - 64, iy + 5, vb,
                    th.palette[b.hw < 0 ? 0 : (b.hw > 4 ? 4 : b.hw)], 9.5f, true);

                // же iconbuttons
                if (button(o, idSeq++, px + cw - 58, iy + 3, 14, 14, it.sameLine ? "L" : "n", it.sameLine)) {
                    it.sameLine = !it.sameLine; o.markDirty();
                }
                if (button(o, idSeq++, px + cw - 42, iy + 3, 14, 14, "g", it.showGraph)) {
                    it.showGraph = !it.showGraph; o.markDirty();
                }
                if (button(o, idSeq++, px + cw - 26, iy + 3, 14, 14, "B", it.big)) {
                    it.big = !it.big; o.markDirty();
                }
                if (button(o, idSeq++, px + cw - 10, iy + 3, 14, 14, "x")) {
                    b.items.erase(b.items.begin() + ii);
                    o.markDirty();
                    iy -= 24; // delete and reuse
                    --ii;
                    continue;
                }
                iy += 24;
            }
            bh = 44 + (float)b.items.size() * 24 + 6;
            // перерисовываем рамку с новой высотой
            fillRect(o, px + 8, cy, cw, bh, 0x0B0F15, 1.f, 6.f);
            strokeRect(o, px + 8.5f, cy + 0.5f, cw - 1, bh - 1, b.enabled ? g_ui.accent : g_ui.border, b.enabled ? 0.35f : 0.15f, 6.f, 1.f);
            cy += bh + 8;

            if (cy > py + ph - 60) break; // не влезает — отладчик скролла
        }

        if (cy > py + ph - 70 && uiScroll == 0) uiScroll = (int)(py + 56 - cy + 40); // автоматическая прокрутка
    }

    /* ==================== ТЕМЫ ==================== */
    if (o.m_cfgTab == 1) {
        uiText(o, px + 8, cy, "ГОТОВЫЕ СТИЛИ ОВЕРЛЕЯ", g_ui.dim, 9.f);
        cy += 16;
        for (int i = 0; i < THEME_COUNT; i++) {
            const ThemeDef& t = THEMES[i];
            bool on = themeIndex(s.theme) == i;
            float th2 = 34.f;
            fillRect(o, px + 8, cy, cw, th2, 0x0B0F15, 1.f, 6.f);
            strokeRect(o, px + 8.5f, cy + 0.5f, cw - 1, th2 - 1, on ? g_ui.accent : g_ui.border, on ? 0.8f : 0.15f, 6.f, 1.f);
            // мини-палитра
            for (int k = 0; k < 5; k++) {
                fillRect(o, px + 12 + k * 7, cy + 8, 5, 5, t.palette[k], 1.f, 1.5f);
            }
            uiText(o, px + 12, cy + 16, t.display, on ? g_ui.accent : g_ui.fg, 10.5f, true);
            const char* hint = "";
            switch (i) {
            case 0: hint = "тёмный + неон"; break;
            case 1: hint = "без подложки, крупный шрифт"; break;
            case 2: hint = "светлое матовое стекло"; break;
            case 3: hint = "чёрный бокс, янтарь"; break;
            case 4: hint = "пурпур, широкие трекинги"; break;
            case 5: hint = "белым по прозрачному"; break;
            case 6: hint = "турнирная полоска"; break;
            case 7: hint = "фосфорный CRT"; break;
            }
            uiText(o, px + 12, cy + 26, hint, g_ui.dim, 8.5f, false, 0.9f);
            if (on) {
                uiText(o, px + cw - 14, cy + 12, "*", g_ui.accent, 12.f, true);
            }
            if (hit(px + 8, cy, cw, th2) && !uiDown && uiWasDown) {
                s.applyTheme(t.id);
                o.m_theme = t;
                o.applySettings(true);
            }
            cy += th2 + 6;
        }
        // направление
        uiText(o, px + 8, cy + 2, "НАПРАВЛЕНИЕ БЛОКОВ", g_ui.dim, 9.f);
        cy += 16;
        float bw2 = (cw - 4) / 2;
        if (button(o, idSeq++, px + 8, cy, bw2, 18, "СТОЛБЦОМ", s.direction == 0)) { s.direction = 0; o.markDirty(); }
        if (button(o, idSeq++, px + 8 + bw2 + 4, cy, bw2, 18, "СТРОКОЙ", s.direction == 1)) { s.direction = 1; o.markDirty(); }
        cy += 26;
    }

    /* ==================== СТИЛЬ ==================== */
    if (o.m_cfgTab == 2) {
        float& fs = s.fontSize;
        sliderRow(o, px + 8, cy, cw, "Размер шрифта", fs, 9, 28, "%.0f"); cy += 26;
        sliderRow(o, px + 8, cy, cw, "Прозрачность", s.opacity, 0.2f, 1.f, "%.2f"); cy += 26;
        sliderRow(o, px + 8, cy, cw, "Заливка фона", s.bgOpacity, 0.f, 1.f, "%.2f"); cy += 26;
        float pad = (float)s.padding;
        if (sliderRow(o, px + 8, cy, cw, "Поля", pad, 2, 22, "%.0f")) s.padding = (int)pad; cy += 26;
        float rg = (float)s.rowGap;
        if (sliderRow(o, px + 8, cy, cw, "Между строками", rg, 0, 12, "%.0f")) s.rowGap = (int)rg; cy += 26;
        float bg = (float)s.blockGap;
        if (sliderRow(o, px + 8, cy, cw, "Между блоками", bg, 0, 24, "%.0f")) s.blockGap = (int)bg; cy += 26;
        float cr = (float)s.cornerRadius;
        if (sliderRow(o, px + 8, cy, cw, "Скругление", cr, 0, 20, "%.0f")) s.cornerRadius = (int)cr; cy += 26;

        cy += 4;
        uiText(o, px + 8, cy, "ЗАГОЛОВКИ БЛОКОВ", g_ui.dim, 9.f); cy += 14;
        float bw3 = (cw - 8) / 3;
        if (button(o, idSeq++, px + 8, cy, bw3, 16, "обычные", s.titleStyle == 0)) { s.titleStyle = 0; o.markDirty(); }
        if (button(o, idSeq++, px + 8 + bw3 + 4, cy, bw3, 16, "плашка", s.titleStyle == 1)) { s.titleStyle = 1; o.markDirty(); }
        if (button(o, idSeq++, px + 8 + 2 * (bw3 + 4), cy, bw3, 16, "полоса", s.titleStyle == 2)) { s.titleStyle = 2; o.markDirty(); }
        cy += 22;

        cy += 2;
        uiText(o, px + 8, cy, "ОПЦИИ", g_ui.dim, 9.f); cy += 14;
        if (toggleRow(o, px + 8, cy, cw, "Рамка", s.showBorder)) o.markDirty(); cy += 20;
        if (toggleRow(o, px + 8, cy, cw, "Разделители", s.showSeparators)) o.markDirty(); cy += 20;
        if (toggleRow(o, px + 8, cy, cw, "Жирный шрифт", s.fontBold)) { o.applySettings(true); } cy += 20;
        if (toggleRow(o, px + 8, cy, cw, "ЗАГЛАВНЫЕ имена", s.uppercase)) o.markDirty(); cy += 20;
        if (toggleRow(o, px + 8, cy, cw, "Тень панели", s.shadow)) o.markDirty(); cy += 20;

        cy += 4;
        uiText(o, px + 8, cy, "ПОЗИЦИЯ", g_ui.dim, 9.f); cy += 14;
        struct { int p; const char* l; } corners[4] = { {0, "◤ ВЛ"}, {1, "◥ ВП"}, {2, "◣ НЛ"}, {3, "◢ НП"} };
        float bw4 = (cw - 4) / 2;
        for (int k = 0; k < 4; k++) {
            float bx = px + 8 + (k & 1) * (bw4 + 4);
            float by = cy + (k >> 1) * 20;
            if (button(o, idSeq++, bx, by, bw4, 18, corners[k].l, s.position == corners[k].p)) {
                s.position = corners[k].p; o.markDirty();
            }
        }
        cy += 44;
        uiText(o, px + 8, cy, "перетащите оверлей мышью — станет custom", g_ui.dim, 8.5f, false, 0.7f);
    }

    /* ==================== КЛАВИШИ ==================== */
    if (o.m_cfgTab == 3) {
        uiText(o, px + 8, cy, "СВОИ СОЧЕТАНИЯ · RegisterHotKey", g_ui.dim, 9.f); cy += 16;
        static const char* hkNames[HK_COUNT] = {
            "Оверлей", "Редактор", "Тема", "Угол", "Сброс AVG/1% LOW", "Захват OBS"
        };
        for (int i = 0; i < HK_COUNT; i++) {
            HotkeyCfg& h = s.hk[i];
            float rowH = 26;
            bool listening = (uiHotkeyListening == i);
            fillRect(o, px + 8, cy, cw, rowH, 0x0B0F15, 1.f, 5.f);
            strokeRect(o, px + 8.5f, cy + 0.5f, cw - 1, rowH - 1, g_ui.border, 0.15f, 5.f, 1.f);
            uiText(o, px + 12, cy + 8, hkNames[i], g_ui.fg, 10.f, false, 0.95f);
            std::string lbl = hkLabel(h);
            float tw2 = o.textW(lbl.c_str(), 9.5f, true);
            if (button(o, idSeq++, px + cw - tw2 - 26, cy + 4, tw2 + 16, 18,
                listening ? "НАЖМИ…" : lbl.c_str(), listening)) {
                uiHotkeyListening = i;
            }
            cy += rowH + 4;
        }
        if (uiHotkeyListening >= 0 && uiCaptureHotkey(s.hk[uiHotkeyListening])) {
            o.markDirty(); o.settings().save(); o.hotkeysDirty = true;
        }
        cy += 6;
        uiText(o, px + 8, cy, "кликните и нажмите комбинацию", g_ui.dim, 8.5f, false, 0.8f);
        cy += 14;

        // сброс на дефолт
        if (button(o, idSeq++, px + 8, cy, cw, 20, "Вернуть стандартные")) {
            s.hk[HK_ToggleOverlay] = { VK_F12, false, true, false };
            s.hk[HK_ToggleConfig]  = { VK_F11, false, true, false };
            s.hk[HK_CycleTheme]    = { VK_F10, false, true, false };
            s.hk[HK_CyclePosition] = { VK_F9,  false, true, false };
            s.hk[HK_ResetStats]    = { VK_F8,  false, true, false };
            s.hk[HK_ToggleCapture] = { VK_F7,  false, true, false };
            o.markDirty(); s.save(); o.hotkeysDirty = true;
        }
        cy += 28;
    }

    /* ==================== СИСТЕМА ==================== */
    if (o.m_cfgTab == 4) {
        // захват OBS
        uiText(o, px + 8, cy, "ЗАХВАТ СТОРОННИМИ ПРОГРАММАМИ", g_ui.dim, 9.f); cy += 16;
        float bw5 = (cw - 4) / 2;
        if (button(o, idSeq++, px + 8, cy, bw5, 22, "РАЗРЕШЁН", s.captureAllow)) { s.captureAllow = true; o.applyCapture(); }
        if (button(o, idSeq++, px + 8 + bw5 + 4, cy, bw5, 22, "СКРЫТ", !s.captureAllow)) { s.captureAllow = false; o.applyCapture(); }
        cy += 30;
        uiText(o, px + 8, cy, s.captureAllow
            ? "оверлей виден в OBS и на скриншотах"
            : "оверлей не попадает в захват (только на вашем экране)", g_ui.dim, 8.5f, false, 0.8f);
        cy += 18;

        // автозапуск
        uiText(o, px + 8, cy, "АВТОЗАПУСК С WINDOWS", g_ui.dim, 9.f); cy += 16;
        if (toggleRow(o, px + 8, cy, cw, "Запускать при старте", s.autostart))
            { s.autostart = !s.autostart; o.markDirty(); } cy += 24;
        // метод
        float bw6 = (cw - 4) / 2;
        if (button(o, idSeq++, px + 8, cy, bw6, 20, "реестр HKCU", !s.autostartTask)) { s.autostartTask = false; o.markDirty(); }
        if (button(o, idSeq++, px + 8 + bw6 + 4, cy, bw6, 20, "планировщик", s.autostartTask)) { s.autostartTask = true; o.markDirty(); }
        cy += 26;
        float delay = (float)s.autostartDelay;
        if (sliderRow(o, px + 8, cy, cw, "Задержка, с", delay, 0, 60, "%.0f")) s.autostartDelay = (int)delay;
        cy += 26;
        if (toggleRow(o, px + 8, cy, cw, "Стартовать свёрнутым", s.autostartMinimized)) o.markDirty();
        cy += 22;
        // применить
        if (button(o, idSeq++, px + 8, cy, cw, 22, "Применить автозапуск")) { o.applyAutostart(); s.save(); }
        cy += 30;

        // сквозные клики / поверх окон / фреймген
        if (toggleRow(o, px + 8, cy, cw, "Сквозные клики (click-through)", s.clickThrough)) o.applyClickThrough();
        cy += 20;
        if (toggleRow(o, px + 8, cy, cw, "Поверх всех окон", s.alwaysOnTop)) o.markDirty();
        cy += 20;
        if (toggleRow(o, px + 8, cy, cw, "Определять генерацию кадров", s.fgDetect, "present-hook — v0.1")) o.markDirty();
        cy += 24;

        // интервалы
        uiText(o, px + 8, cy, "ОПРОС ЖЕЛЕЗА", g_ui.dim, 9.f); cy += 14;
        float upd = (float)s.updateIntervalMs;
        if (sliderRow(o, px + 8, cy, cw, "Система", upd, 100, 2000, "%.0f")) s.updateIntervalMs = (int)upd;
        cy += 26;
        float fpsI = (float)s.fpsIntervalMs;
        if (sliderRow(o, px + 8, cy, cw, "FPS", fpsI, 50, 1000, "%.0f")) s.fpsIntervalMs = (int)fpsI;
        cy += 26;

        // ресурсы
        cy += 8;
        fillRect(o, px + 8, cy, cw, 52, 0x0A0F14, 1.f, 6.f);
        strokeRect(o, px + 8.5f, cy + 0.5f, cw - 1, 51, g_ui.accent, 0.3f, 6.f, 1.f);
        char rss[48];
        PROCESS_MEMORY_COUNTERS pmc{};
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
            sprintf_s(rss, "ОЗУ %.1f МБ", (double)pmc.WorkingSetSize / 1048576.0);
        else sprintf_s(rss, "ОЗУ ?.?? МБ");
        uiText(o, px + 14, cy + 8, rss, g_ui.accent, 11.f, true);
        uiText(o, px + 14, cy + 24, "CPU < 0.5 % · 1 процесс · layered окно · Immediate-UI", g_ui.dim, 8.5f, false, 0.9f);
        uiText(o, px + 14, cy + 36, "нет второго процесса GUI", g_ui.dim, 8.5f, false, 0.7f);
    }

    uiEndFrame();
}
