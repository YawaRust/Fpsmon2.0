#pragma once
#include "../common.h"
#include "../config/Model.h"

class Overlay;

/*
 * ConfigPanelWeb — браузерное окно настроек на WebView2.
 * Если SDK недоступен (HAVE_WEBVIEW2=0), все методы — заглушки.
 */

class ConfigPanelWeb {
public:
    explicit ConfigPanelWeb(Overlay* parent);
    ~ConfigPanelWeb();

    bool create(HWND parentHwnd);
    void destroy();
    void show(bool visible);
    void sendSettings(const Settings& s);
    bool isReady() const;

    Overlay* m_parent = nullptr;
    HWND     m_hwnd   = nullptr;

    struct Impl;
    Impl* m_impl = nullptr;
};
