#pragma once
#include "../common.h"
#include "../config/Model.h"
#include "../core/Sensors.h"

/*
 * Layered-окно с per-pixel alpha: рендерим в 32bpp DIB через
 * ID2D1DCRenderTarget и выводим UpdateLayeredWindow — так получаем
 * настоящую прозрачность и скруглённые углы без артефактов.
 */

class Overlay {
public:
    bool create();
    void destroy();

    void poll(float dtMs);          // управляет видимостью, таймингом отрисовки
    void repaint();                  // немедленная перерисовка

    // действия хоткеев / трея
    void toggleVisible();
    void toggleConfig();
    void cycleTheme();
    void cyclePosition();
    void toggleCapture();

    void applySettings(bool rereadPos);
    void applyCapture();
    void applyClickThrough();
    void applyAutostart();
    void clearAutostart();

    HWND hwnd() const { return m_hwnd; }

    // контекст для ConfigUI
    ID2D1DCRenderTarget* rt() const { return m_rt; }
    SystemMonitor& monitor() { return m_mon; }
    Settings& settings() { return m_set; }
    void markDirty() { m_dirty = true; }

    // текстовые хелперы DWrite (используются и рендером, и конфиг-панелью)
    void text(float x, float y, const char* s, uint32_t rgb, float size, bool bold = false, float alpha = 1.f);
    float textW(const char* s, float size, bool bold = false);

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT onMsg(UINT msg, WPARAM, LPARAM);

    void draw();

    // измерение содержимого
    float measure(int& outW, int& outH);
    SIZE positionFor(int w, int h);
    bool ensureTarget(int w, int h);

    // helper рисования
    IDWriteTextFormat* fmt(float size, bool bold);
    D2D1_COLOR_F palColor(int hw, float alpha = 1.f) const;

    Settings m_set;
    SystemMonitor m_mon;
    ThemeDef m_theme{};

    HWND m_hwnd = nullptr;
    bool m_visible = true;
    bool m_configOpen = false;
    bool m_dirty = true;

    // GDI/D2D
    ID2D1Factory* m_factory = nullptr;
    ID2D1DCRenderTarget* m_rt = nullptr;
    IDWriteFactory* m_dw = nullptr;
    IDWriteTextFormat* m_fmt = nullptr;
    IDWriteTextFormat* m_fmtBig = nullptr;
    IDWriteTextFormat* m_fmtSmall = nullptr;
    HDC m_memDC = nullptr;
    HBITMAP m_dib = nullptr;
    HGDIOBJ m_oldBmp = nullptr;
    void* m_bits = nullptr;
    int m_w = 0, m_h = 0;
    void* m_dibBits = nullptr; // alias стабильности

    // кеш шрифтов: пересоздаём при смене семейства/размера
    wchar_t m_fontCacheW[64] = {};
    float m_fontCacheSize = -1.f;
    void recreateFonts(const wchar_t* family, float fSize, bool boldTheme);

    // положение оверлея на экране
    int m_x = 14, m_y = 14;

    // перетаскивание мышью
    bool m_drag = false;
    POINT m_dragStart{};

    // IM UI состояние (живёт в ConfigUI.cpp)
    int m_imHover = 0, m_imActive = 0;
    int m_cfgScroll = 0;
    int m_cfgTab = 0;
    int m_captureHk = -1;   // индекс слушаемого хоткея, -1 = нет
    int m_cfgPanelX = 0;    // x конфиг-панели внутри окна

public:
    bool hotkeysDirty = false; // true → main перерегистрирует хоткеи

    friend void drawConfigUI(Overlay&);
};

/* обработчик кодов трей-меню, регистрируется из main.cpp */
extern void (*g_trayMsgHandler)(int id);
