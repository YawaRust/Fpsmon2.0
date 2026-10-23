#pragma once

// Порядок важен: макросы должны быть до windows.h
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dxgi1_4.h>
#include <pdh.h>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdarg>

// PDH_MORE_DATA не всегда экспортируется из pdh.h в зависимости от _WIN32_WINNT
#ifndef PDH_MORE_DATA
#  define PDH_MORE_DATA ((DWORD)0x000007D2L)
#endif

#ifndef APP_VERSION
#  define APP_VERSION "0.0.2"
#endif

// ─── D2D helpers ────────────────────────────────────────────────────────────

inline D2D1_COLOR_F toColorF(uint32_t rgb, float a = 1.f) {
    return D2D1::ColorF(
        float((rgb >> 16) & 0xFF) / 255.f,
        float((rgb >>  8) & 0xFF) / 255.f,
        float( rgb        & 0xFF) / 255.f,
        a);
}

inline uint32_t rgbFromHex(const char* hex) {
    if (!hex) return 0;
    const char* h = (hex[0] == '#') ? hex + 1 : hex;
    return (uint32_t)strtoul(h, nullptr, 16) & 0xFFFFFF;
}

// ─── String helpers ──────────────────────────────────────────────────────────

inline std::wstring utf8ToWide(const char* s) {
    if (!s || !s[0]) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (n <= 1) return L"";
    std::wstring w(size_t(n - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), n);
    return w;
}

inline std::string wideToUtf8(const wchar_t* w) {
    if (!w || !w[0]) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 1) return "";
    std::string s(size_t(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
    return s;
}

inline std::string wideToUtf8(const std::wstring& w) {
    return wideToUtf8(w.c_str());
}

inline std::wstring wideUpper(std::wstring s) {
    for (auto& c : s) c = (wchar_t)towupper(c);
    return s;
}

// ─── Misc ────────────────────────────────────────────────────────────────────

inline void clampf(float& v, float lo, float hi) {
    if (v < lo) v = lo; else if (v > hi) v = hi;
}
