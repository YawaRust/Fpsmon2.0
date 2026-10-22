#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dxgi1_4.h>
#include <pdh.h>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdio>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")

#ifndef APP_VERSION
#define APP_VERSION "0.0.1"
#endif

/* в некоторых SDK константа объявлена только при определённом _WIN32_WINNT */
#ifndef PDH_MORE_DATA
#define PDH_MORE_DATA ((DWORD)0x000007D2L)
#endif

inline uint32_t rgbFromHex(const char* hex) {
    if (!hex || !hex[0]) return 0xFFFFFF;
    const char* h = hex[0] == '#' ? hex + 1 : hex;
    return (uint32_t)strtoul(h, nullptr, 16) & 0xFFFFFF;
}

inline D2D1_COLOR_F toColorF(uint32_t rgb, float a = 1.f) {
    return D2D1::ColorF(
        (float)((rgb >> 16) & 0xFF) / 255.f,
        (float)((rgb >> 8) & 0xFF) / 255.f,
        (float)(rgb & 0xFF) / 255.f, a);
}

inline void hexFromRgb(uint32_t rgb, char* out, size_t n) {
    sprintf_s(out, n, "#%06X", rgb & 0xFFFFFF);
}

inline void clampf(float& v, float a, float b) { v = v < a ? a : (v > b ? b : v); }
