#include "ConfigPanelWeb.h"
#include "Overlay.h"
#include "EmbeddedUi.h"
#include "../utils/Logger.h"

#if defined(HAVE_WEBVIEW2) && HAVE_WEBVIEW2

#include <WebView2.h>
#include <wrl.h>
using namespace Microsoft::WRL;


static bool jsonDouble(const wchar_t* json, const wchar_t* key, double& out) {
    wchar_t pat[64];
    swprintf_s(pat, L"\"%s\":", key);
    const wchar_t* p = wcsstr(json, pat);
    if (!p) return false;
    p += wcslen(pat);
    while (*p == L' ' || *p == L'\t') p++;
    wchar_t* end = nullptr;
    out = wcstod(p, &end);
    return end != p;
}

static bool jsonBool(const wchar_t* json, const wchar_t* key, bool& out) {
    wchar_t pat[64];
    swprintf_s(pat, L"\"%s\":", key);
    const wchar_t* p = wcsstr(json, pat);
    if (!p) return false;
    p += wcslen(pat);
    while (*p == L' ' || *p == L'\t') p++;
    if (wcsncmp(p, L"true", 4) == 0) { out = true; return true; }
    if (wcsncmp(p, L"false", 5) == 0) { out = false; return true; }
    return false;
}

static bool jsonString(const wchar_t* json, const wchar_t* key, wchar_t* out, size_t n) {
    wchar_t pat[64];
    swprintf_s(pat, L"\"%s\":\"", key);
    const wchar_t* p = wcsstr(json, pat);
    if (!p) return false;
    p += wcslen(pat);
    size_t i = 0;
    while (p[i] && p[i] != L'"' && i + 1 < n) { out[i] = p[i]; i++; }
    out[i] = L'\0';
    return i > 0;
}

struct ConfigPanelWeb::Impl {
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2>           webview;
    bool ready = false;
};

ConfigPanelWeb::ConfigPanelWeb(Overlay* p) : m_parent(p) { m_impl = new Impl(); }
ConfigPanelWeb::~ConfigPanelWeb() { destroy(); delete m_impl; }

bool ConfigPanelWeb::create(HWND parentHwnd) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"YawaConfigPanel";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"YawaFPS HUD",
        WS_POPUP | WS_CLIPCHILDREN,
        100, 100, 400, 620,
        parentHwnd, nullptr, wc.hInstance, nullptr);
    if (!m_hwnd) return false;

    // Папка данных WebView2
    wchar_t ap[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, ap);
    std::wstring dataDir = std::wstring(ap) + L"\\YawaFPSHUD\\webview2";

    auto self = this;

    CreateCoreWebView2EnvironmentWithOptions(nullptr, dataDir.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [self](HRESULT, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(self->m_hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [self](HRESULT, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (!ctrl) return E_FAIL;
                            self->m_impl->controller = ctrl;
                            ctrl->get_CoreWebView2(&self->m_impl->webview);

                            ComPtr<ICoreWebView2Settings> cfg;
                            self->m_impl->webview->get_Settings(&cfg);
                            cfg->put_AreDefaultContextMenusEnabled(FALSE);
                            cfg->put_IsZoomControlEnabled(FALSE);

                            self->m_impl->webview->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [self](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR json = nullptr;
                                        args->get_WebMessageAsJson(&json);
                                        if (json) {
                                            Settings& s = self->m_parent->settings();
                                            if (wcsstr(json, L"\"type\":\"close\"")) {
                                                s.save();
                                                self->m_parent->toggleConfig();
                                            } else if (wcsstr(json, L"\"type\":\"update\"")) {
                                                double v = 0;
                                                if (jsonDouble(json, L"fontSize", v)) s.fontSize = (float)v;
                                                if (jsonDouble(json, L"opacity", v)) s.opacity = (float)v;
                                                if (jsonDouble(json, L"bgOpacity", v)) s.bgOpacity = (float)v;
                                                if (jsonDouble(json, L"padding", v)) s.padding = (int)v;
                                                if (jsonDouble(json, L"rowGap", v)) s.rowGap = (int)v;
                                                if (jsonDouble(json, L"blockGap", v)) s.blockGap = (int)v;
                                                if (jsonDouble(json, L"updateInterval", v)) s.updateIntervalMs = (int)v;
                                                if (jsonDouble(json, L"fpsInterval", v)) s.fpsIntervalMs = (int)v;
                                                bool b = false;
                                                if (jsonBool(json, L"showBorder", b)) s.showBorder = b;
                                                if (jsonBool(json, L"fontBold", b)) s.fontBold = b;
                                                if (jsonBool(json, L"clickThrough", b)) s.clickThrough = b;
                                                if (jsonBool(json, L"alwaysOnTop", b)) s.alwaysOnTop = b;
                                                if (jsonBool(json, L"rounded", b)) {
                                                    if (b && s.cornerRadius == 0) s.cornerRadius = 10;
                                                    if (!b) s.cornerRadius = 0;
                                                }
                                                wchar_t buf[64] = {};
                                                if (jsonString(json, L"theme", buf, 64)) {
                                                    std::string t = wideToUtf8(buf);
                                                    if (!t.empty()) s.applyTheme(t);
                                                }
                                                if (jsonString(json, L"direction", buf, 64)) {
                                                    s.direction = (wcscmp(buf, L"row") == 0) ? 1 : 0;
                                                }
                                                char cbuf[16] = {};
                                                if (jsonString(json, L"captureMode", buf, 64)) {
                                                    std::string cm = wideToUtf8(buf);
                                                    s.captureAllow = (cm == "allow");
                                                }
                                                if (jsonDouble(json, L"gpuIndex", v))
                                                    s.gpuIndex = (int)v;
                                                self->m_parent->applySettings(false);
                                            }
                                            CoTaskMemFree(json);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            self->m_impl->webview->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [self](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*) -> HRESULT {
                                        self->m_impl->ready = true;
                                        self->sendSettings(self->m_parent->settings());
                                        if (self->m_pendingShow) {
                                            self->m_pendingShow = false;
                                            ShowWindow(self->m_hwnd, SW_SHOW);
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            std::wstring html = utf8ToWide(kEmbeddedUiHtml);
                            self->m_impl->webview->NavigateToString(html.c_str());

                            // Размер WebView = размер окна
                            RECT r; GetClientRect(self->m_hwnd, &r);
                            self->m_impl->controller->put_Bounds(r);

                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    return true;
}

void ConfigPanelWeb::show(bool v) {
    if (!m_hwnd) return;
    if (v) {
        // Позиционируем рядом с оверлеем, чтобы не уйти за край экрана
        RECT or_; GetWindowRect(m_parent->hwnd(), &or_);
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int px = or_.right + 14;
        if (px + 400 > sw) px = or_.left - 400 - 14;
        if (px < 0) px = 14;
        int py = or_.top;
        if (py + 620 > GetSystemMetrics(SM_CYSCREEN)) py = GetSystemMetrics(SM_CYSCREEN) - 620 - 14;
        if (py < 0) py = 14;
        SetWindowPos(m_hwnd, HWND_TOPMOST, px, py, 400, 620, SWP_NOACTIVATE);
        if (m_impl && m_impl->controller) {
            RECT r; GetClientRect(m_hwnd, &r);
            m_impl->controller->put_Bounds(r);
        }
    }
    ShowWindow(m_hwnd, v ? SW_SHOWNOACTIVATE : SW_HIDE);
    if (v) {
        SetForegroundWindow(m_hwnd);
        if (m_impl && m_impl->controller) m_impl->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }
}

bool ConfigPanelWeb::isReady() const { return m_impl && m_impl->ready; }

void ConfigPanelWeb::sendSettings(const Settings& s) {
    if (!m_impl || !m_impl->webview || !m_impl->ready) return;
    std::wstring gpuNames = L"[";
    const auto& adapters = m_parent->monitor().gpuNames();
    for (size_t i = 0; i < adapters.size(); ++i) {
        if (i) gpuNames += L",";
        gpuNames += L"\"";
        for (wchar_t ch : adapters[i]) {
            if (ch == L'\\' || ch == L'\"') gpuNames += L'\\';
            gpuNames += ch;
        }
        gpuNames += L"\"";
    }
    gpuNames += L"]";

    wchar_t json[8192];
    swprintf_s(json,
        L"{\"type\":\"settings\",\"fontSize\":%.0f,\"opacity\":%.2f,\"bgOpacity\":%.2f,"
        L"\"padding\":%d,\"rowGap\":%d,\"blockGap\":%d,"
        L"\"showBorder\":%s,\"rounded\":%s,\"fontBold\":%s,"
        L"\"direction\":%d,\"theme\":\"%S\","
        L"\"clickThrough\":%s,\"alwaysOnTop\":%s,\"captureAllow\":%s,"
        L"\"gpuIndex\":%d,\"gpuNames\":%s}",
        s.fontSize, s.opacity, s.bgOpacity,
        s.padding, s.rowGap, s.blockGap,
        s.showBorder ? L"true" : L"false",
        s.cornerRadius > 0 ? L"true" : L"false",
        s.fontBold ? L"true" : L"false",
        s.direction, s.theme.c_str(),
        s.clickThrough ? L"true" : L"false",
        s.alwaysOnTop ? L"true" : L"false",
        s.captureAllow ? L"true" : L"false",
        s.gpuIndex, gpuNames.c_str());
    m_impl->webview->PostWebMessageAsJson(json);
}

void ConfigPanelWeb::destroy() {
    if (m_impl && m_impl->controller) { m_impl->controller->Close(); m_impl->controller = nullptr; }
    if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
}

#else
// ─── Заглушка ────────────────────────────────────────────────────────────────
ConfigPanelWeb::ConfigPanelWeb(Overlay* p) : m_parent(p) {}
ConfigPanelWeb::~ConfigPanelWeb() {}
bool ConfigPanelWeb::create(HWND) { LOG("WebView2 SDK not available"); return false; }
void ConfigPanelWeb::destroy() {}
void ConfigPanelWeb::show(bool) {}
void ConfigPanelWeb::sendSettings(const Settings&) {}
bool ConfigPanelWeb::isReady() const { return false; }
#endif
