#pragma once
#include "../common.h"
#include "../config/Model.h"
#include "../core/Sensors.h"

/*
 * Layered-окно с per-pixel alpha.
 * Рендер через ID2D1DCRenderTarget → UpdateLayeredWindow.
 */

class Overlay {
public:
    bool create();
    void destroy();

    void poll(float dtMs);
    void repaint();

    void toggleVisible();
    void toggleConfig();
    void cycleTheme();
    void cyclePosition();
    void toggleCapture();

    void applySettings(bool rereadPos);
    void applyCapture();
    void applyClickThrough();
    void applyAutostart();

    HWND hwnd() const { return m_hwnd; }

    // ── public API ───────────────────────────────────────────────────────
    ID2D1DCRenderTarget* rt()       const { return m_rt; }
    SystemMonitor&       monitor()        { return m_mon; }
    Settings&            settings()       { return m_set; }
    void                 markDirty()      { m_dirty = true; }

    void  text (float x, float y, const wchar_t* s, uint32_t rgb,
                float size, bool bold = false, float alpha = 1.f);
    float textW(const wchar_t* s, float size, bool bold = false);

    int  m_cfgPanelX = 0;
    int  m_cfgTab    = 0;
    int  m_w = 0, m_h = 0;

    bool hotkeysDirty  = false;
    bool m_lmbDown     = false;
    bool m_lmbWasDown  = false;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT onMsg(UINT, WPARAM, LPARAM);

    void   draw();
    float  measure(int& outW, int& outH);
    SIZE   positionFor(int w, int h);
    bool   ensureTarget(int w, int h);

    IDWriteTextFormat* fmt(float size, bool bold);
    void recreateFonts(const wchar_t* family, float sz, bool boldTheme);

    Settings      m_set;
    SystemMonitor m_mon;
    ThemeDef      m_theme{};

    HWND m_hwnd = nullptr;
    bool m_visible     = true;
    bool m_configOpen  = false;
    bool m_configRestoreVisible = true;
    bool m_dirty       = true;

    ID2D1Factory*         m_factory  = nullptr;
    ID2D1DCRenderTarget*  m_rt       = nullptr;
    IDWriteFactory*       m_dw       = nullptr;
    IDWriteTextFormat*    m_fmt      = nullptr;
    IDWriteTextFormat*    m_fmtBig   = nullptr;
    IDWriteTextFormat*    m_fmtSmall = nullptr;

    HDC     m_memDC  = nullptr;
    HBITMAP m_dib    = nullptr;
    HGDIOBJ m_oldBmp = nullptr;
    void*   m_bits   = nullptr;

    wchar_t m_fontCacheW[64] = {};
    float   m_fontCacheSize  = -1.f;

    int   m_x = 14, m_y = 14;
    bool  m_drag      = false;
    POINT m_dragStart{};

};

extern void (*g_trayMsgHandler)(int id);
