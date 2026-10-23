#include "common.h"
#include "utils/Logger.h"
#include "config/Model.h"
#include "overlay/Overlay.h"

/*
 * Точка входа YawaFPS HUD v0.0.1
 *
 *   --autostart        запуск из автозагрузки (без окна-флага)
 *   --tray             стартовать со скрытым оверлеем
 *   --reset-config     сбросить config.ini и выйти
 *
 * Один процесс: окно-оверлей (layered, D2D), трей-иконка с меню,
 * менеджер глобальных хоткеев. Приоритет процесса BELOW_NORMAL.
 */

static const UINT WM_TRAYICON = WM_APP + 42;
static const int  IDM_EXIT = 1001;
static const int  IDM_CONFIG = 1002;
static const int  IDM_TOGGLE = 1003;

static HINSTANCE  g_inst = nullptr;
static Overlay    g_ov;
static NOTIFYICONDATAA g_nid{};
static HMENU       g_trayMenu = nullptr;

/* трей callback вызывается из Overlay WndProc (WM_APP+ид сообщение иконки) */

/* ---------- трей ---------- */

static void trayAdd() {
    NOTIFYICONDATAW nidW{};
    nidW.cbSize = sizeof(nidW);
    nidW.hWnd = g_ov.hwnd();
    nidW.uID = 1;
    nidW.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nidW.uCallbackMessage = WM_TRAYICON;
    nidW.hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!nidW.hIcon) nidW.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nidW.szTip, L"YawaFPS HUD");
    Shell_NotifyIconW(NIM_ADD, &nidW);
}

static void trayRemove() {
    NOTIFYICONDATAW nidW{};
    nidW.cbSize = sizeof(nidW);
    nidW.hWnd = g_ov.hwnd();
    nidW.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nidW);
}

static void trayMenuBuild() {
    if (g_trayMenu) DestroyMenu(g_trayMenu);
    g_trayMenu = CreatePopupMenu();
    AppendMenuW(g_trayMenu, MF_STRING, IDM_TOGGLE, L"Показать / скрыть (Shift+F12)");
    AppendMenuW(g_trayMenu, MF_STRING, IDM_CONFIG, L"Настройки (Shift+F11)");
    AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_trayMenu, MF_STRING, IDM_EXIT, L"Выход");
}

/* ---------- хоткеи ---------- */

static void registerHotkeys() {
    Settings& s = g_ov.settings();
    for (int i = 0; i < HK_COUNT; i++) {
        UnregisterHotKey(g_ov.hwnd(), 100 + i);
        const HotkeyCfg& h = s.hk[i];
        if (h.vk)
            RegisterHotKey(g_ov.hwnd(), 100 + i, hkMods(h) | MOD_NOREPEAT, h.vk);
    }
}

/* ---------- WndProc для трея (в окне оверлея) ---------- */

static void onTrayMsg(int id) {
    Settings& s = g_ov.settings();
    switch (id) {
    case IDM_TOGGLE: g_ov.toggleVisible(); break;
    case IDM_CONFIG: g_ov.toggleConfig(); break;
    case IDM_EXIT:
        s.save();
        trayRemove();
        DestroyWindow(g_ov.hwnd());
        break;
    default: break;
    }
}

/* -1 → показать контекстное меню трея · -2 → двойной клик */
static void trayHandler(int id) {
    if (id == -1) {
        if (!g_trayMenu) trayMenuBuild();
        POINT pt; GetCursorPos(&pt);
        SetForegroundWindow(g_ov.hwnd());
        int cmd = TrackPopupMenu(g_trayMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
            pt.x, pt.y, 0, g_ov.hwnd(), nullptr);
        PostMessageW(g_ov.hwnd(), WM_NULL, 0, 0);
        onTrayMsg(cmd);
        return;
    }
    if (id == -2) { g_ov.toggleConfig(); return; }
    onTrayMsg(id);
}

/* ---------- вход ---------- */

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR cmdLine, int) {
    g_inst = hinst;

    // один экземпляр
    HANDLE hMutex = CreateMutexA(nullptr, TRUE, "yawafps_hud_single");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (cmdLine && strstr(cmdLine, "--autostart")) {
            // повторный автозапуск не нужен
        } else {
            HWND other = FindWindowW(L"YawaSideOverlay", nullptr);
            if (other) PostMessageW(other, WM_APP + 100, 0, 0); // поднять конфиг
            CloseHandle(hMutex);
            return 0;
        }
    }

    // аргументы
    bool autostart = cmdLine && strstr(cmdLine, "--autostart");
    bool tray = cmdLine && strstr(cmdLine, "--tray");
    bool resetCfg = cmdLine && strstr(cmdLine, "--reset-config");

    // приоритет
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

    // DPI
    HMODULE u32 = GetModuleHandleA("user32.dll");
    if (u32) {
        typedef BOOL(WINAPI* SDPAC)(void*);
        auto fn = (SDPAC)GetProcAddress(u32, "SetProcessDpiAwarenessContext");
        if (fn) fn((void*)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    }

    // лог
    std::string lp = logPath();
    Logger::instance().init(lp.c_str());
    LOGF("=== YawaFPS HUD %s запущен ===", APP_VERSION);

    if (resetCfg) {
        DeleteFileA(configPath().c_str());
        LOG("config.ini удалён по запросу");
        ReleaseMutex(hMutex); CloseHandle(hMutex);
        return 0;
    }

    // оверлей
    if (!g_ov.create()) {
        LOG("не удалось инициализировать оверлей");
        MessageBoxA(nullptr, "Не удалось инициализировать оверлей (D2D/окно).", "YawaFPS HUD", MB_ICONERROR);
        ReleaseMutex(hMutex); CloseHandle(hMutex);
        return 1;
    }

    if (tray) g_ov.toggleVisible(); // скрыть оверлей, если стартовали свёрнутым
    (void)autostart;

    // трей
    trayAdd();
    trayMenuBuild();

    // хоткеи
    registerHotkeys();

    // таймер ~60 Гц для отзывчивого UI/drag; реальные метрики опрашиваются своими интервалами
    SetTimer(g_ov.hwnd(), 1, 16, nullptr);

    // трей-меню обрабатываем из Overlay::onMsg через глобальный callback
    g_trayMsgHandler = trayHandler;

    LOG("всё готово, жду сообщений");

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (g_ov.hotkeysDirty) {
            g_ov.hotkeysDirty = false;
            registerHotkeys();
        }
    }

    // выход
    KillTimer(g_ov.hwnd(), 1);
    for (int i = 0; i < HK_COUNT; i++) UnregisterHotKey(g_ov.hwnd(), 100 + i);
    trayRemove();
    if (g_trayMenu) { DestroyMenu(g_trayMenu); g_trayMenu = nullptr; }
    g_ov.destroy();
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    LOG("=== выход ===");
    return (int)msg.wParam;
}
