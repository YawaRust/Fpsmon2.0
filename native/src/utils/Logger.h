#pragma once
#include <cstdio>
#include <cstdarg>
#include <mutex>

/*
 * Минимальный логгер: один файл рядом с exe, без фреймворков.
 * Выключается, если рядом нет config.ini (dev-режим).
 */

class Logger {
public:
    static Logger& instance() { static Logger l; return l; }

    void init(const char* path) {
        std::lock_guard<std::mutex> g(m_mtx);
        m_fp = nullptr;
        fopen_s(&m_fp, path, "a");
    }

    void log(const char* msg) {
        std::lock_guard<std::mutex> g(m_mtx);
        if (!m_fp) return;
        SYSTEMTIME st; GetLocalTime(&st);
        fprintf(m_fp, "[%02d:%02d:%02d.%03d] %s\n",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
        fflush(m_fp);
    }

    void logf(const char* fmt, ...) {
        char buf[512];
        va_list ap; va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        log(buf);
    }

    ~Logger() { if (m_fp) fclose(m_fp); }

private:
    Logger() = default;
    FILE* m_fp = nullptr;
    std::mutex m_mtx;
};

#define LOG(msg)  Logger::instance().log(msg)
#define LOGF(...) Logger::instance().logf(__VA_ARGS__)
