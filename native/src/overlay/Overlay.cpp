#include "Overlay.h"
#include "../utils/Logger.h"

void drawConfigUI(Overlay& o); // ConfigUI.cpp

void (*g_trayMsgHandler)(int) = nullptr;

static const wchar_t* CLS_NAME = L"YawaSideOverlay";

/* формат значения, как в веб-прототипе */
static void fmtValue(char* out, size_t n, float v, int dec) {
    if (v >= 10000.f) sprintf_s(out, n, "%.0f", v);
    else if (dec == 0) sprintf_s(out, n, "%.0f", v);
    else sprintf_s(out, n, "%.*f", dec, v);
}

/* ---------- DPI ---------- */
static int dpiOf(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) ReleaseDC(hwnd, dc);
    return dpi > 0 ? dpi : 96;
}

/* ============ окно ============ */

bool Overlay::create() {
    m_set.load();
    m_theme = m_set.themeDef();
    m_mon.init();
    m_visible = true; // --tray (свёрнутый старт) обрабатывает main.cpp

    // D2D заводы
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory);
    if (FAILED(hr)) { LOG("D2D factory: ошибка"); return false; }

    const D2D1_RENDER_TARGET_PROPERTIES rtp = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0,
        D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);
    hr = m_factory->CreateDCRenderTarget(&rtp, &m_rt);
    if (FAILED(hr)) { LOG("D2D DC target: ошибка"); return false; }

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory), (IUnknown**)&m_dw);

    // окно: layered, topmost, tool, без захвата фокуса
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    DWORD ex = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    if (m_set.clickThrough) ex |= WS_EX_TRANSPARENT;

    m_hwnd = CreateWindowExW(ex, CLS_NAME, L"YawaFPS HUD",
        WS_POPUP, 100, 100, 240, 200, nullptr, nullptr,
        GetModuleHandle(nullptr), this);
    if (!m_hwnd) { LOG("окно overlay не создано"); return false; }

    // шрифты темы — кеш; при смене размера/темы пересоздаются в draw()
    m_fontCacheSize = -1.f;

    applyCapture();
    if (m_visible) ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);

    LOGF("Overlay создан: тема=%s, видим=%d", m_theme.id, m_visible ? 1 : 0);
    return true;
}

void Overlay::destroy() {
    if (m_hwnd) DestroyWindow(m_hwnd);
    m_hwnd = nullptr;
    m_mon.shutdown();
    if (m_fmt) m_fmt->Release();
    if (m_fmtBig) m_fmtBig->Release();
    if (m_fmtSmall) m_fmtSmall->Release();
    if (m_rt) m_rt->Release();
    if (m_dw) m_dw->Release();
    if (m_factory) m_factory->Release();
    m_fmt = m_fmtBig = m_fmtSmall = nullptr;
    m_rt = nullptr; m_dw = nullptr; m_factory = nullptr;
}

/* ============ доступ ============ */

void Overlay::recreateFonts(const wchar_t* family, float fSize, bool boldTheme) {
    wcsncpy_s(m_fontCacheW, family, 63);
    m_fontCacheSize = fSize;
    if (m_fmt) m_fmt->Release();
    if (m_fmtBig) m_fmtBig->Release();
    if (m_fmtSmall) m_fmtSmall->Release();
    m_dw->CreateTextFormat(family, nullptr,
        boldTheme ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        (std::max)(6.f, fSize * 0.98f), L"en-us", &m_fmt);
    m_dw->CreateTextFormat(family, nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        (std::max)(6.f, fSize), L"en-us", &m_fmtBig);
    m_dw->CreateTextFormat(family, nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        (std::max)(6.f, fSize * 0.68f), L"en-us", &m_fmtSmall);
}

IDWriteTextFormat* Overlay::fmt(float size, bool bold) {
    const float f = m_set.fontSize;
    if (bold) {
        const float big = f * 1.42f;
        return (size > f * 1.2f || big == size) ? m_fmtBig : m_fmtBig;
    }
    const float fSmall = f * 0.75f;
    return (size <= fSmall) ? m_fmtSmall : m_fmt;
}

D2D1_COLOR_F Overlay::palColor(int hw, float alpha) const {
    hw = hw < 0 ? 0 : (hw > 4 ? 4 : hw);
    return toColorF(m_theme.palette[hw], alpha);
}

/* ============ Helpers рисования ============ */

void Overlay::text(float x, float y, const char* s, uint32_t rgb, float size, bool bold, float alpha) {
    IDWriteTextFormat* f = fmt(size, bold);
    wchar_t wbuf[128];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, wbuf, 127);
    if (wlen <= 0) return;

    IDWriteTextLayout* layout = nullptr;
    if (FAILED(m_dw->CreateTextLayout(wbuf, wlen - 1, f, 300.f, (std::max)(12.f, size * 1.6f), &layout)))
        return;

    ID2D1SolidColorBrush* br = nullptr;
    m_rt->CreateSolidColorBrush(toColorF(rgb, alpha), &br);
    if (br) {
        m_rt->DrawTextLayout(D2D1::Point2F(x, y), layout, br,
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        br->Release();
    }
    layout->Release();
}

float Overlay::textW(const char* s, float size, bool bold) {
    IDWriteTextFormat* f = fmt(size, bold);
    wchar_t wbuf[128];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, wbuf, 127);
    if (wlen <= 1) return 0.f;
    IDWriteTextLayout* layout = nullptr;
    if (FAILED(m_dw->CreateTextLayout(wbuf, wlen - 1, f, 600.f, (std::max)(12.f, size * 1.6f), &layout)))
        return 0.f;
    DWRITE_TEXT_METRICS tm{};
    layout->GetMetrics(&tm);
    layout->Release();
    return tm.width;
}

/* ============ измерение и позиционирование ============ */

float Overlay::measure(int& outW, int& outH) {
    const float f = m_set.fontSize;
    const float lineH = f * 1.22f + m_set.rowGap;
    float wMax = 0.f, h = (float)m_set.padding;
    const Settings& s = m_set;

    for (const BlockCfg& b : s.blocks) {
        if (!b.enabled || b.items.empty()) continue;
        float bw = 0.f;
        float by = 0.f;

        // заголовок блока
        if (b.showTitle && !b.title.empty())
            by += f * 0.72f + 2.f;

        // строки блока
        float curLineW = 0.f;
        bool firstInLine = true;
        for (size_t ii = 0; ii < b.items.size(); ii++) {
            const ItemCfg& it = b.items[ii];
            const SensorInfo& si = sensorInfo(it.sensor);

            float iw = 0.f;
            if (it.showLabel && !it.label.empty())
                iw += textW(it.label.c_str(), f, false) + f * 0.5f;
            char vbuf[32]; sprintf_s(vbuf, "999.9");
            iw += textW(vbuf, it.big ? f * 1.42f : f * 0.98f, true);
            if (it.showUnit && si.unit[0]) iw += f * 0.32f + textW(si.unit, f * 0.68f, false);
            if (it.showGraph) iw += f * 3.6f + f * 0.4f;

            if (!it.sameLine || firstInLine) {
                if (!firstInLine && curLineW > bw) bw = curLineW;
                if (!firstInLine) { by += lineH; }
                else { /* первая строка блока */ }
                curLineW = iw;
                firstInLine = false;
            } else {
                curLineW += f * 0.62f + iw;
            }
        }
        if (curLineW > bw) bw = curLineW;
        by += lineH; // последняя строка

        if (bw > wMax) wMax = bw;
        h += by + (float)m_set.blockGap;
    }
    h += (float)m_set.padding;

    // конфиг-панель увеличивает высоту окна? Нет — рисуем оверлей целиком в одном DIB
    // и конфиг-панель справа от него? Для простоты: конфиг рендерится в отдельной области окна
    if (m_configOpen) { outW = (int)ceil(wMax + m_set.padding * 2); outH = (int)ceil(h); }
    else { outW = (int)ceil(wMax + m_set.padding * 2); outH = (int)ceil(h); }
    return wMax;
}

SIZE Overlay::positionFor(int w, int h) {
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SIZE p{};
    switch (m_set.position) {
    case 1: p.cx = sw - w - 14; p.cy = 14; break;
    case 2: p.cx = 14; p.cy = sh - h - 48; break;
    case 3: p.cx = sw - w - 14; p.cy = sh - h - 48; break;
    case 4: p.cx = m_set.customX; p.cy = m_set.customY; break;
    default: p.cx = 14; p.cy = 14; break;
    }
    return p;
}

bool Overlay::ensureTarget(int w, int h) {
    if (w == m_w && h == m_h && m_dib) return true;
    if (m_oldBmp && m_memDC) SelectObject(m_memDC, m_oldBmp);
    if (m_dib) DeleteObject(m_dib);
    if (m_memDC) DeleteDC(m_memDC);
    m_dib = nullptr; m_memDC = nullptr; m_oldBmp = nullptr;
    m_w = w; m_h = h;

    HDC screen = GetDC(nullptr);
    m_memDC = CreateCompatibleDC(screen);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_dib = (HBITMAP)CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    ReleaseDC(nullptr, screen);
    if (!m_dib) { DeleteDC(m_memDC); m_memDC = nullptr; return false; }
    m_oldBmp = SelectObject(m_memDC, m_dib);
    return true;
}

/* ============ отрисовка ============ */

void Overlay::draw() {
    if (!m_visible && !m_configOpen) return;

    // пересоздаём шрифты при смене темы или размера
    {
        wchar_t famNow[64];
        MultiByteToWideChar(CP_UTF8, 0, m_theme.font, -1, famNow, 64);
        if (m_fontCacheSize != m_set.fontSize || wcsncmp(m_fontCacheW, famNow, 63) != 0)
            recreateFonts(famNow, m_set.fontSize, m_theme.bold);
    }

    int baseW = 0, baseH = 0;
    measure(baseW, baseH);
    if (baseW < 10) baseW = 10;
    if (baseH < 10) baseH = 10;

    int needW = baseW, needH = baseH;
    m_cfgPanelX = baseW;
    if (m_configOpen) {
        needW = baseW + 312;
        if (needH < 380) needH = 380;
    }
    if (!ensureTarget(needW, needH)) return;

    RECT rc = { 0, 0, needW, needH };
    HRESULT hr = m_rt->BindDC(m_memDC, &rc);
    if (FAILED(hr)) return;

    m_rt->BeginDraw(); // возвращает void
    m_rt->SetDpi((float)dpiOf(m_hwnd), (float)dpiOf(m_hwnd));
    m_rt->Clear(D2D1::ColorF(0, 0, 0, 0));

    const float f = m_set.fontSize;
    const auto& th = m_theme;
    const float pad = (float)m_set.padding;

    // фон
    {
        ID2D1SolidColorBrush* bg = nullptr;
        m_rt->CreateSolidColorBrush(toColorF(th.bg, m_set.bgOpacity), &bg);
        D2D1_ROUNDED_RECT rr;
        rr.rect = D2D1::RectF(0.5f, 0.5f, needW - 0.5f, needH - 0.5f);
        rr.radiusX = (float)m_set.cornerRadius; rr.radiusY = (float)m_set.cornerRadius;
        m_rt->FillRoundedRectangle(&rr, bg);
        if (m_set.showBorder) {
            ID2D1SolidColorBrush* bb = nullptr;
            m_rt->CreateSolidColorBrush(toColorF(th.border, 0.3f), &bb);
            m_rt->DrawRoundedRectangle(&rr, bb, 1.f);
            if (bb) bb->Release();
        }
        if (bg) bg->Release();
    }

    // блоки
    float y = pad;
    const float lineH = f * 1.22f + m_set.rowGap;
    const float x0 = pad;
    const Settings& s = m_set;
    int blockIdx = 0;
    int envLine = 0;

    for (const BlockCfg& b : s.blocks) {
        if (!b.enabled || b.items.empty()) continue;

        // разделитель блока
        if (blockIdx > 0 && s.showSeparators) {
            ID2D1SolidColorBrush* sep = nullptr;
            m_rt->CreateSolidColorBrush(toColorF(th.border, 0.22f), &sep);
            m_rt->DrawLine(D2D1::Point2F(x0, y - s.blockGap / 2.f),
                D2D1::Point2F(needW - pad, y - s.blockGap / 2.f), sep, 1.f);
            if (sep) sep->Release();
        }

        // заголовок блока
        if (b.showTitle && !b.title.empty()) {
            std::string t = s.uppercase ? b.title : b.title;
            if (s.uppercase) {
                for (auto& c : t) c = (char)toupper((unsigned char)c);
            }
            uint32_t tc = th.label;
            float a = 0.72f;
            text(x0, y, t.c_str(), tc, f * 0.62f, true, a);
            y += f * 0.8f;
        }

        // строки
        float x = x0;
        float baseY = y;
        (void)baseY;
        bool firstInLine = true;
        bool newLine = true;
        float curX = x0;

        for (size_t ii = 0; ii < b.items.size(); ii++) {
            const ItemCfg& it = b.items[ii];
            const SensorInfo& si = sensorInfo(it.sensor);
            if (!si.available) continue; // не умеем измерять — не показываем

            if (newLine && it.sameLine && !firstInLine) {
                // продолжить строку
            } else {
                if (!firstInLine) { y += lineH; curX = x0; }
                firstInLine = false;
            }

            // имя
            if (it.showLabel && !it.label.empty()) {
                std::string lb = it.label;
                if (s.uppercase) for (auto& c : lb) c = (char)toupper((unsigned char)c);
                text(curX, y, lb.c_str(), th.label, f * 0.68f, false, 0.78f);
                curX += textW(lb.c_str(), f * 0.68f, false) + f * 0.5f;
            }

            // значение
            char vbuf[32];
            float val = m_mon.value(it.sensor);
            fmtValue(vbuf, sizeof(vbuf), val, si.decimals);
            uint32_t vc = th.palette[b.hw < 0 ? 0 : (b.hw > 4 ? 4 : b.hw)];
            // пороговые цвета: warn — янтарь, crit — красный
            uint32_t warnC = rgbFromHex("#ffd23f"), critC = rgbFromHex("#ff3b5c");
            if (si.warnLow) {
                if (it.crit > 0 && val <= it.crit) vc = critC;
                else if (it.warn > 0 && val <= it.warn) vc = warnC;
            } else {
                if (it.crit > 0 && val >= it.crit) vc = critC;
                else if (it.warn > 0 && val >= it.warn) vc = warnC;
            }
            float vsz = it.big ? f * 1.42f : f * 0.98f;
            text(curX, y, vbuf, vc, vsz, true);
            curX += textW(vbuf, vsz, true);

            // единица измерения
            if (it.showUnit && si.unit[0]) {
                curX += f * 0.32f;
                text(curX, y, si.unit, vc, f * 0.68f, false, 0.55f);
                curX += textW(si.unit, f * 0.68f, false) + f * 0.4f;
            }

            // мини-график
            if (it.showGraph && m_mon.histSize() > 4) {
                float gx = needW - pad - f * 3.6f; // фиксированная ширина справа
                float gy = y + f * 0.14f, gw = f * 3.6f, gh = f * 0.82f;
                ID2D1SolidColorBrush* gbg = nullptr;
                m_rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1, 0.06f), &gbg);
                D2D1_RECT_F gr = D2D1::RectF(gx, gy, gx + gw, gy + gh);
                m_rt->FillRectangle(&gr, gbg); gbg->Release();

                const auto& hi = m_mon;
                int n = hi.histSize();
                const float* hist = hi.hist(it.sensor);
                ID2D1SolidColorBrush* gls = nullptr;
                m_rt->CreateSolidColorBrush(toColorF(th.palette[b.hw < 0 ? 0 : (b.hw > 4 ? 4 : b.hw)], 0.9f), &gls);
                if (gls) {
                    int start = hi.histHead();
                    int prevIdx = -1;
                    float maxv = 1.f;
                    for (int k = 0; k < n; k++) {
                        int idx = (start + k) % HISTORY_SIZE;
                        if (hist[idx] > maxv) maxv = hist[idx];
                    }
                    float stepX = n > 1 ? gw / (n - 1) : 0;
                    for (int k = 0; k < n; k++) {
                        int idx = (start + k) % HISTORY_SIZE;
                        float v = hist[idx] / maxv;
                        if (v < 0.f) v = 0; if (v > 1.f) v = 1;
                        float px = gx + k * stepX;
                        float py = gy + gh - v * gh;
                        if (prevIdx >= 0) {
                            float px0 = gx + (k - 1) * stepX;
                            int pidx = (start + k - 1) % HISTORY_SIZE;
                            float pv = hist[pidx] / maxv;
                            if (pv < 0.f) pv = 0; if (pv > 1.f) pv = 1;
                            float py0 = gy + gh - pv * gh;
                            m_rt->DrawLine(D2D1::Point2F(px0, py0), D2D1::Point2F(px, py), gls, 1.15f);
                        }
                        prevIdx = idx;
                    }
                    gls->Release();
                }
            }
            envLine++;
        }
        // конец последней строки блока
        if (!firstInLine) y += lineH;
        y += (float)s.blockGap;
        blockIdx++;
    }

    // конфиг-панель рисуется справа от оверлея в том же окне
    if (m_configOpen) drawConfigUI(*this);

    hr = m_rt->EndDraw();

    // вывод UpdateLayeredWindow
    SIZE pos = positionFor(needW, needH);
    m_x = pos.cx; m_y = pos.cy;
    SetWindowPos(m_hwnd, m_set.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        pos.cx, pos.cy, needW, needH, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    HDC screen = GetDC(nullptr);
    POINT ptSrc = { 0, 0 };
    POINT ptDst = { pos.cx, pos.cy };
    SIZE szWin = { needW, needH };
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = (BYTE)(m_set.opacity * 255);
    blend.AlphaFormat = AC_SRC_ALPHA;
    UpdateLayeredWindow(m_hwnd, screen, &ptDst, &szWin, m_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    if (screen) ReleaseDC(nullptr, screen);
}

void Overlay::repaint() { m_dirty = true; }

void Overlay::poll(float dt) {
    static int histAcc = 0;
    histAcc += (int)dt;
    if (histAcc >= 100) { histAcc -= 100; m_mon.pushHistory(); }

    if (m_dirty) { m_dirty = false; draw(); }
}

/* ============ действия ============ */

void Overlay::toggleVisible() {
    m_visible = !m_visible;
    if (!m_visible && !m_configOpen) {
        // layered-окно скрываем полностью
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
            SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        m_dirty = true;
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    }
}

void Overlay::toggleConfig() {
    m_configOpen = !m_configOpen;
    LONG ex = GetWindowLongA(m_hwnd, GWL_EXSTYLE);
    if (m_configOpen) {
        // при открытом редакторе окно должно принимать ввод
        SetWindowLongA(m_hwnd, GWL_EXSTYLE, ex & ~WS_EX_TRANSPARENT);
        if (!m_visible) { m_visible = true; ShowWindow(m_hwnd, SW_SHOWNOACTIVATE); }
    } else {
        SetWindowLongA(m_hwnd, GWL_EXSTYLE, m_set.clickThrough ? (ex | WS_EX_TRANSPARENT) : (ex & ~WS_EX_TRANSPARENT));
        m_set.save();
        hotkeysDirty = true;
        applySettings(false);
    }
    m_dirty = true;
}

void Overlay::cycleTheme() {
    int i = themeIndex(m_set.theme);
    i = (i + 1) % THEME_COUNT;
    m_set.applyTheme(THEMES[i].id);
    m_theme = m_set.themeDef();
    m_dirty = true;
}

void Overlay::cyclePosition() {
    m_set.position = (m_set.position + 1) % 4; // tl→tr→br→bl
    m_dirty = true;
}

void Overlay::toggleCapture() {
    m_set.captureAllow = !m_set.captureAllow;
    applyCapture();
}

void Overlay::applyCapture() {
    if (!m_hwnd) return;
    // WDA_EXCLUDEFROMCAPTURE требует Windows 10 2004+; на старых вернёт ошибку
    SetWindowDisplayAffinity(m_hwnd,
        m_set.captureAllow ? WDA_NONE : WDA_EXCLUDEFROMCAPTURE);
    m_dirty = true;
}

void Overlay::applyClickThrough() {
    if (!m_hwnd) return;
    LONG ex = GetWindowLongA(m_hwnd, GWL_EXSTYLE);
    if (m_set.clickThrough && !m_configOpen) ex |= WS_EX_TRANSPARENT; else ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongA(m_hwnd, GWL_EXSTYLE, ex);
    m_dirty = true;
}

void Overlay::applySettings(bool rereadPos) {
    m_theme = m_set.themeDef();
    m_set.sanitize();
    m_fontCacheSize = -1.f; // пересоздастся в draw() с учётом новой темы
    applyClickThrough();
    applyCapture();
    if (rereadPos) m_dirty = true;
}

/* ---------- автозапуск ---------- */

void Overlay::applyAutostart() {
    if (m_set.autostartApplied == m_set.autostart) return; // уже синхронизировано

    char exePath[MAX_PATH]; GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    if (m_set.autostartTask) {
        // планировщик задач (может потребовать права)
        char cmd[1024];
        sprintf_s(cmd,
            "schtasks /create /f /tn \"YawaFPS HUD\" /tr \"\\\"%s\\\" --autostart%s\" /sc onlogon /delay %02d:%02d /rl limited",
            exePath, m_set.autostartMinimized ? " --tray" : "",
            m_set.autostartDelay, 0);
        if (m_set.autostart) {
            int rc = (int)(INT_PTR)ShellExecuteA(nullptr, "open", "cmd.exe",
                (std::string("/c ") + cmd).c_str(), nullptr, SW_HIDE);
            if (rc > 32) { m_set.autostartApplied = true; }
        } else {
            ShellExecuteA(nullptr, "open", "cmd.exe", "/c schtasks /delete /f /tn \"YawaFPS HUD\"", nullptr, SW_HIDE);
            m_set.autostartApplied = false;
        }
    } else {
        HKEY rk = nullptr;
        if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &rk) == ERROR_SUCCESS) {
            if (m_set.autostart) {
                std::string val = std::string("\"") + exePath + "\"" +
                    (m_set.autostartMinimized ? " --autostart --tray" : " --autostart");
                RegSetValueExA(rk, "YawaFPSHUD", 0, REG_SZ,
                    (const BYTE*)val.c_str(), (DWORD)(val.size() + 1));
                m_set.autostartApplied = true;
            } else {
                RegDeleteValueA(rk, "YawaFPSHUD");
                m_set.autostartApplied = false;
            }
            RegCloseKey(rk);
        }
    }
    m_set.save();
}

void Overlay::clearAutostart() {
    m_set.autostart = false;
    applyAutostart();
}

/* ============ WndProc ============ */

LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return TRUE;
    }
    auto* o = (Overlay*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!o) return DefWindowProcW(hwnd, msg, wParam, lParam);
    return o->onMsg(msg, wParam, lParam);
}

LRESULT Overlay::onMsg(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND: return 1;

    case WM_TIMER: {
        if (wp == 1) {
            m_mon.pollFast();
            static int sysCounter = 0;
            sysCounter += 50;
            if (sysCounter >= m_set.updateIntervalMs) {
                sysCounter = 0;
                m_mon.pollSys();
            }
            poll(50);
        }
        return 0;
    }

    case WM_HOTKEY: {
        int id = (int)wp;
        switch (id) {
        case 100 + HK_ToggleOverlay: toggleVisible(); break;
        case 100 + HK_ToggleConfig:  toggleConfig(); break;
        case 100 + HK_CycleTheme:    cycleTheme(); break;
        case 100 + HK_CyclePosition: cyclePosition(); break;
        case 100 + HK_ResetStats:    m_mon.resetStats(); repaint(); break;
        case 100 + HK_ToggleCapture: toggleCapture(); break;
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (m_configOpen) { m_imActive = 1; }
        if (!m_set.clickThrough || m_configOpen) {
            m_drag = true;
            SetCapture(m_hwnd);
            GetCursorPos(&m_dragStart);
        }
        repaint();
        return 0;
    }
    case WM_LBUTTONUP: {
        if (m_set.clickThrough && !m_configOpen) return DefWindowProcW(m_hwnd, msg, wp, lp);
        m_drag = false; ReleaseCapture();
        if (m_imActive) { m_imActive = 0; repaint(); }
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (m_drag) {
            POINT pt; GetCursorPos(&pt);
            RECT r; GetWindowRect(m_hwnd, &r);
            int nx = (int)(r.left + (pt.x - m_dragStart.x));
            int ny = (int)(r.top + (pt.y - m_dragStart.y));
            m_set.customX = nx; m_set.customY = ny;
            m_set.position = 4;
            repaint();
        }
        return 0;
    }
    case WM_RBUTTONUP:
        toggleConfig();
        return 0;

    case WM_DISPLAYCHANGE:
        repaint();
        return 0;

    default:
        // сообщение трей-иконки (WM_APP+basis, приходит с uID через lParam/ид)
        if (msg >= WM_APP && msg <= 0xBFFF && g_trayMsgHandler) {
            // FFIS раскладка: (LOWORD(lp) = событие мыши, HIWORD(lp) = иконка id)
            if (LOWORD((DWORD)lp) == WM_RBUTTONUP || LOWORD((DWORD)lp) == NIN_SELECT) {
                POINT pt; GetCursorPos(&pt);
                // меню трея регистрируется из main через статическую глобальную; вызываем через handler
                // просто вызываем handler с кодом -1 — сигнал показать меню
                g_trayMsgHandler(-1);
                return 0;
            }
            if (LOWORD((DWORD)lp) == WM_LBUTTONDBLCLK) {
                g_trayMsgHandler(-2);
                return 0;
            }
        }
        break;
    }
    return DefWindowProcW(m_hwnd, msg, wp, lp);
}
