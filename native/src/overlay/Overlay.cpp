#include "Overlay.h"
#include "ConfigPanelWeb.h"
#include "../utils/Logger.h"

void (*g_trayMsgHandler)(int) = nullptr;

static const UINT WM_TRAYICON = WM_APP + 42;

static const wchar_t* CLS_NAME = L"YawaFPSHUD_Overlay";

static int dpiOf(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    int d = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) ReleaseDC(hwnd, dc);
    return d > 0 ? d : 96;
}

static void fmtValue(char* out, size_t n, float v, int dec) {
    if (v >= 10000.f) sprintf_s(out, n, "%.0f", v);
    else              sprintf_s(out, n, "%.*f", dec, v);
}

void Overlay::recreateFonts(const wchar_t* family, float sz, bool boldTheme) {
    wcsncpy_s(m_fontCacheW, family, 63);
    m_fontCacheSize = sz;

    auto rel = [](IDWriteTextFormat*& p) { if (p) { p->Release(); p = nullptr; } };
    rel(m_fmt); rel(m_fmtBig); rel(m_fmtSmall);

    m_dw->CreateTextFormat(family, nullptr, boldTheme ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, sz, L"en-us", &m_fmt);
    m_dw->CreateTextFormat(family, nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, sz * 1.42f, L"en-us", &m_fmtBig);
    m_dw->CreateTextFormat(family, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, sz * 0.7f, L"en-us", &m_fmtSmall);
}

IDWriteTextFormat* Overlay::fmt(float size, bool bold) {
    float sz = m_set.fontSize;
    if (bold) return m_fmtBig;
    if (size <= sz * 0.75f) return m_fmtSmall;
    return m_fmt;
}

void Overlay::text(float x, float y, const wchar_t* s, uint32_t rgb, float size, bool bold, float alpha) {
    if (!s || !s[0] || !m_rt || !m_dw) return;
    IDWriteTextFormat* f = fmt(size, bold);
    UINT32 len = (UINT32)wcslen(s);
    IDWriteTextLayout* layout = nullptr;
    if (FAILED(m_dw->CreateTextLayout(s, len, f, 1200.f, (std::max)(size * 2.5f, 14.f), &layout))) return;
    DWRITE_TEXT_RANGE rng{0, len};
    layout->SetFontSize(size, rng);
    if (bold) layout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, rng);
    ID2D1SolidColorBrush* br = nullptr;
    m_rt->CreateSolidColorBrush(toColorF(rgb, alpha), &br);
    if (br) {
        m_rt->DrawTextLayout(D2D1::Point2F(x, y), layout, br, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        br->Release();
    }
    layout->Release();
}

float Overlay::textW(const wchar_t* s, float size, bool bold) {
    if (!s || !s[0] || !m_dw) return 0.f;
    IDWriteTextFormat* f = fmt(size, bold);
    UINT32 len = (UINT32)wcslen(s);
    IDWriteTextLayout* layout = nullptr;
    if (FAILED(m_dw->CreateTextLayout(s, len, f, 1200.f, (std::max)(size * 2.5f, 14.f), &layout))) return 0.f;
    DWRITE_TEXT_RANGE rng{0, len};
    layout->SetFontSize(size, rng);
    DWRITE_TEXT_METRICS tm{};
    layout->GetMetrics(&tm);
    layout->Release();
    return tm.width;
}

bool Overlay::create() {
    m_set.load();
    m_theme = m_set.themeDef();
    m_mon.init(m_set.gpuIndex);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory);
    D2D1_RENDER_TARGET_PROPERTIES rtp = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    m_factory->CreateDCRenderTarget(&rtp, &m_rt);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_dw);

    wchar_t fam[64];
    MultiByteToWideChar(CP_UTF8, 0, m_theme.font, -1, fam, 64);
    recreateFonts(fam, m_set.fontSize, m_theme.bold);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, CLS_NAME, L"YawaFPS HUD", WS_POPUP, 100, 100, 300, 200, nullptr, nullptr, wc.hInstance, this);
    
    applyCapture();
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    return true;
}

void Overlay::destroy() {
    if (m_configWeb) delete m_configWeb;
    if (m_hwnd) DestroyWindow(m_hwnd);
    if (m_rt) m_rt->Release();
    if (m_dw) m_dw->Release();
    if (m_factory) m_factory->Release();
}

bool Overlay::ensureTarget(int w, int h) {
    if (w == m_w && h == m_h && m_dib) return true;
    if (m_dib) DeleteObject(m_dib);
    if (m_memDC) DeleteDC(m_memDC);
    m_w = w; m_h = h;
    HDC sc = GetDC(nullptr);
    m_memDC = CreateCompatibleDC(sc);
    BITMAPINFO bi = { sizeof(bi.bmiHeader), w, -h, 1, 32, BI_RGB };
    m_dib = CreateDIBSection(sc, &bi, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    ReleaseDC(nullptr, sc);
    SelectObject(m_memDC, m_dib);
    return true;
}

float Overlay::measure(int& outW, int& outH) {
    float f = m_set.fontSize, lineH = f * 1.22f + m_set.rowGap, wMax = 0, h = (float)m_set.padding;
    for (const auto& b : m_set.blocks) {
        if (!b.enabled || b.items.empty()) continue;
        float bw = 0, by = 0;
        if (b.showTitle && !b.title.empty()) by += f * 0.72f + 2;
        float cw = 0; bool first = true;
        for (const auto& it : b.items) {
            float iw = 0;
            if (it.showLabel && !it.label.empty()) iw += textW(it.label.c_str(), f, false) + f * 0.5f;
            iw += textW(L"999.9", it.big ? f * 1.42f : f * 0.98f, true);
            if (!it.sameLine || first) { if (!first) by += lineH; cw = iw; first = false; } else cw += f * 0.62f + iw;
            if (cw > bw) bw = cw;
        }
        by += lineH; if (bw > wMax) wMax = bw;
        h += by + (float)m_set.blockGap;
    }
    outW = (int)ceil(wMax + m_set.padding * 2); outH = (int)ceil(h + m_set.padding);
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

void Overlay::draw() {
    int bw, bh; measure(bw, bh);
    ensureTarget(bw, bh);
    RECT rc = { 0, 0, bw, bh };
    m_rt->BindDC(m_memDC, &rc);
    m_rt->BeginDraw();
    m_rt->Clear(D2D1::ColorF(0, 0, 0, 0));

    ID2D1SolidColorBrush* bgBr;
    m_rt->CreateSolidColorBrush(toColorF(m_theme.bg, m_set.bgOpacity), &bgBr);
    D2D1_ROUNDED_RECT rr = { D2D1::RectF(0, 0, (float)bw, (float)bh), (float)m_set.cornerRadius, (float)m_set.cornerRadius };
    m_rt->FillRoundedRectangle(&rr, bgBr);
    bgBr->Release();

    float y = (float)m_set.padding;
    for (const auto& b : m_set.blocks) {
        if (!b.enabled || b.items.empty()) continue;
        if (b.showTitle) { text(m_set.padding, y, b.title.c_str(), m_theme.label, m_set.fontSize * 0.7f, true); y += m_set.fontSize; }
        bool first = true; float cx = (float)m_set.padding;
        for (const auto& it : b.items) {
            if (!it.sameLine || first) { if (!first) y += m_set.fontSize * 1.2f; cx = (float)m_set.padding; first = false; }
            wchar_t val[32]; swprintf_s(val, L"%.1f", m_mon.value(it.sensor));
            text(cx, y, val, m_theme.palette[b.hw], m_set.fontSize, it.big);
            cx += textW(val, m_set.fontSize, it.big) + 10;
        }
        y += m_set.fontSize * 1.2f + m_set.blockGap;
    }

    m_rt->EndDraw();
    SIZE pos = positionFor(bw, bh);
    SetWindowPos(m_hwnd, HWND_TOPMOST, pos.cx, pos.cy, bw, bh, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    HDC sc = GetDC(nullptr); POINT ps = { 0, 0 }, pd = { pos.cx, pos.cy }; SIZE sz = { bw, bh };
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)(m_set.opacity * 255), AC_SRC_ALPHA };
    UpdateLayeredWindow(m_hwnd, sc, &pd, &sz, m_memDC, &ps, 0, &bf, ULW_ALPHA);
    ReleaseDC(nullptr, sc);
}

void Overlay::poll(float dt) { m_mon.pollFast(); static int sa = 0; sa += 16; if (sa >= m_set.updateIntervalMs) { sa = 0; m_mon.pollSys(); } draw(); }
void Overlay::repaint() { m_dirty = true; }

void Overlay::toggleVisible() { m_visible = !m_visible; ShowWindow(m_hwnd, m_visible ? SW_SHOWNOACTIVATE : SW_HIDE); }

void Overlay::toggleConfig() {
    m_configOpen = !m_configOpen;
    if (m_configOpen) {
        if (!m_configWeb) { m_configWeb = new ConfigPanelWeb(this); m_configWeb->create(m_hwnd); }
        m_configWeb->show(true);
    } else {
        if (m_configWeb) { m_configWeb->show(false); }
        m_set.save();
        hotkeysDirty = true;
        applySettings(false);
    }
}

void Overlay::applyCapture() {
    if (!m_hwnd) return;
    SetWindowDisplayAffinity(m_hwnd, m_set.captureAllow ? WDA_NONE : WDA_EXCLUDEFROMCAPTURE);
}

void Overlay::applyClickThrough() {
    if (!m_hwnd) return;
    LONG ex = GetWindowLongW(m_hwnd, GWL_EXSTYLE);
    if (m_set.clickThrough && !m_configOpen) ex |= WS_EX_TRANSPARENT;
    else ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongW(m_hwnd, GWL_EXSTYLE, ex);
    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void Overlay::applySettings(bool rereadPos) {
    m_theme = m_set.themeDef();
    m_set.sanitize();
    m_fontCacheSize = -1.f;
    applyClickThrough();
    applyCapture();
    if (m_mon.selectedGpu() != m_set.gpuIndex) m_mon.setSelectedGpu(m_set.gpuIndex);
    // Синхронизируем WebView2 при изменении настроек из C++
    if (m_configWeb && m_configWeb->isReady()) m_configWeb->sendSettings(m_set);
    if (rereadPos) m_dirty = true;
}

void Overlay::applyAutostart() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    HKEY rk = nullptr;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_WRITE, &rk) == ERROR_SUCCESS) {
        if (m_set.autostart) {
            std::string val = std::string("\"") + exePath + "\" --autostart";
            if (m_set.autostartMinimized) val += " --tray";
            RegSetValueExA(rk, "YawaFPSHUD", 0, REG_SZ, (BYTE*)val.c_str(), (DWORD)(val.size() + 1));
        } else {
            RegDeleteValueA(rk, "YawaFPSHUD");
        }
        RegCloseKey(rk);
    }
    m_set.autostartApplied = m_set.autostart;
    m_set.save();
}

LRESULT CALLBACK Overlay::WndProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_NCCREATE) { SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW*)lp)->lpCreateParams); return 1; }
    Overlay* o = (Overlay*)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (!o) return DefWindowProcW(h, m, wp, lp);
    if (m == WM_TIMER) { o->poll(16); return 0; }
    if (m == WM_TRAYICON) {
        if (g_trayMsgHandler) {
            if (lp == WM_RBUTTONUP || lp == WM_CONTEXTMENU) g_trayMsgHandler(-1);
            else if (lp == WM_LBUTTONDBLCLK) g_trayMsgHandler(-2);
        }
        return 0;
    }
    if (m == WM_HOTKEY) { if (wp == 101) o->toggleConfig(); if (wp == 100) o->toggleVisible(); return 0; }
    if (m == WM_LBUTTONDOWN) { o->m_drag = true; SetCapture(h); GetCursorPos(&o->m_dragStart); return 0; }
    if (m == WM_LBUTTONUP) {
        o->m_drag = false; ReleaseCapture();
        o->settings().save();
        return 0;
    }
    if (m == WM_MOUSEMOVE && o->m_drag) {
        POINT pt; GetCursorPos(&pt); RECT r; GetWindowRect(h, &r);
        int nx = r.left + pt.x - o->m_dragStart.x;
        int ny = r.top + pt.y - o->m_dragStart.y;
        SetWindowPos(h, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        o->m_dragStart = pt;
        o->settings().position = 4; // custom
        o->settings().customX = nx;
        o->settings().customY = ny;
        return 0;
    }
    return DefWindowProcW(h, m, wp, lp);
}
