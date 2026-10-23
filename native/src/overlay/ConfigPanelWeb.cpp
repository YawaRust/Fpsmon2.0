#include "ConfigPanelWeb.h"
#include "Overlay.h"
#include "EmbeddedUi.h"
#include "../utils/Logger.h"

#if defined(HAVE_WEBVIEW2) && HAVE_WEBVIEW2

#include <WebView2.h>
#include <wrl.h>
using namespace Microsoft::WRL;

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
                                            // Простой парсинг
                                            Settings& s = self->m_parent->settings();
                                            if (wcsstr(json, L"\"type\":\"close\"")) {
                                                s.save();
                                                self->m_parent->toggleConfig();
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
        // Позиционируем справа от оверлея
        RECT or_; GetWindowRect(m_parent->hwnd(), &or_);
        int px = or_.right + 14;
        int py = or_.top;
        SetWindowPos(m_hwnd, HWND_TOPMOST, px, py, 400, 620, SWP_NOACTIVATE);
        if (m_impl && m_impl->controller) {
            RECT r; GetClientRect(m_hwnd, &r);
            m_impl->controller->put_Bounds(r);
        }
    }
    ShowWindow(m_hwnd, v ? SW_SHOW : SW_HIDE);
}

bool ConfigPanelWeb::isReady() const { return m_impl && m_impl->ready; }

void ConfigPanelWeb::sendSettings(const Settings& s) {
    if (!m_impl || !m_impl->webview || !m_impl->ready) return;
    wchar_t json[4096];
    swprintf_s(json,
        L"{\"type\":\"settings\",\"fontSize\":%.0f,\"opacity\":%.2f,\"bgOpacity\":%.2f,"
        L"\"padding\":%d,\"rowGap\":%d,\"blockGap\":%d,"
        L"\"showBorder\":%s,\"rounded\":%s,\"fontBold\":%s,"
        L"\"direction\":%d,\"theme\":\"%S\","
        L"\"clickThrough\":%s,\"alwaysOnTop\":%s,\"captureAllow\":%s}",
        s.fontSize, s.opacity, s.bgOpacity,
        s.padding, s.rowGap, s.blockGap,
        s.showBorder ? L"true" : L"false",
        s.cornerRadius > 0 ? L"true" : L"false",
        s.fontBold ? L"true" : L"false",
        s.direction, s.theme.c_str(),
        s.clickThrough ? L"true" : L"false",
        s.alwaysOnTop ? L"true" : L"false",
        s.captureAllow ? L"true" : L"false");
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
