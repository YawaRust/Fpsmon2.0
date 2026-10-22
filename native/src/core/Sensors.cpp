#include "Sensors.h"
#include "../utils/Logger.h"

static uint64_t fileTime64(const FILETIME& ft) {
    return ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

/* ---------- PDH helpers ---------- */

double SystemMonitor::pdhScalar(PDH_HCOUNTER c) {
    if (!c) return 0.0;
    PDH_FMT_COUNTERVALUE v;
    if (PdhGetFormattedCounterValue(c, PDH_FMT_DOUBLE, nullptr, &v) != ERROR_SUCCESS)
        return 0.0;
    return v.doubleValue;
}

double SystemMonitor::pdhMaxArray(PDH_HCOUNTER c) {
    if (!c) return 0.0;
    DWORD size = 0, count = 0;
    if (PdhGetFormattedCounterArrayA(c, PDH_FMT_DOUBLE, &size, &count, nullptr)
        != PDH_MORE_DATA || size == 0)
        return 0.0;
    std::vector<BYTE> buf(size);
    auto* arr = (PDH_FMT_COUNTERVALUE_ITEM_A*)buf.data();
    if (PdhGetFormattedCounterArrayA(c, PDH_FMT_DOUBLE, &size, &count, arr) != ERROR_SUCCESS)
        return 0.0;
    double mx = 0.0;
    for (DWORD i = 0; i < count; i++)
        if (arr[i].FmtValue.CStatus == ERROR_SUCCESS && arr[i].FmtValue.doubleValue > mx)
            mx = arr[i].FmtValue.doubleValue;
    return mx;
}

double SystemMonitor::pdhSumArray(PDH_HCOUNTER c) {
    if (!c) return 0.0;
    DWORD size = 0, count = 0;
    if (PdhGetFormattedCounterArrayA(c, PDH_FMT_DOUBLE, &size, &count, nullptr)
        != PDH_MORE_DATA || size == 0)
        return 0.0;
    std::vector<BYTE> buf(size);
    auto* arr = (PDH_FMT_COUNTERVALUE_ITEM_A*)buf.data();
    if (PdhGetFormattedCounterArrayA(c, PDH_FMT_DOUBLE, &size, &count, arr) != ERROR_SUCCESS)
        return 0.0;
    double sum = 0.0;
    for (DWORD i = 0; i < count; i++)
        if (arr[i].FmtValue.CStatus == ERROR_SUCCESS)
            sum += arr[i].FmtValue.doubleValue;
    return sum;
}

/* ---------- init ---------- */

bool SystemMonitor::init() {
    QueryPerformanceFrequency(&m_freq);
    GetSystemTimes(&m_prevIdle, &m_prevKernel, &m_prevUser);

    // DXGI 1.4 → VRAM
    IDXGIFactory1* fac1 = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&fac1)))) {
        m_factory = fac1;
        IDXGIAdapter1* ad = nullptr;
        if (SUCCEEDED(fac1->EnumAdapters1(0, &ad))) {
            IDXGIAdapter3* a3 = nullptr;
            if (SUCCEEDED(ad->QueryInterface(IID_PPV_ARGS(&a3)))) {
                m_adapter3 = a3;
                DXGI_ADAPTER_DESC1 desc{};
                if (SUCCEEDED(ad->GetDesc1(&desc)))
                    m_vramTotalMB = (float)(desc.DedicatedVideoMemory / (1024 * 1024));
            }
            ad->Release();
        }
    }
    LOGF("DXGI: адаптер %s, VRAM total %.0f MB", m_adapter3 ? "OK" : "нет", m_vramTotalMB);

    // PDH
    if (PdhOpenQueryA(nullptr, 0, &m_q) != ERROR_SUCCESS) {
        LOG("PDH: не удалось открыть запрос");
    } else {
        PdhAddEnglishCounterA(m_q, "\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &m_cGpu);
        PdhAddEnglishCounterA(m_q, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &m_cDiskR);
        PdhAddEnglishCounterA(m_q, "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &m_cDiskW);
        PdhAddEnglishCounterA(m_q, "\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_cDiskT);
        PdhAddEnglishCounterA(m_q, "\\Network Interface(*)\\Bytes Sent/sec", 0, &m_cNetS);
        PdhAddEnglishCounterA(m_q, "\\Network Interface(*)\\Bytes Received/sec", 0, &m_cNetR);
        PdhAddEnglishCounterA(m_q, "\\Thermal Zone Information(*)\\Temperature", 0, &m_cThermal);

        // первый сбор — счётчики rate дают валидные значения со второго
        PdhCollectQueryData(m_q);
        Sleep(60);
        PdhCollectQueryData(m_q);
        m_pdhReady = true;

        double t = pdhMaxArray(m_cThermal);
        m_hasTemp = m_cThermal != nullptr && t > 200.0 && t < 400.0;
        LOGF("PDH: готов, тепловые зоны %s", m_hasTemp ? "найдены" : "не найдены");
    }

    if (m_vramTotalMB > 0)
        m_v[(int)SensorId::VRAMPct] = 0;

    LOG("SystemMonitor инициализирован");
    return true;
}

void SystemMonitor::shutdown() {
    if (m_q) { PdhCloseQuery(m_q); m_q = nullptr; }
    if (m_adapter3) { m_adapter3->Release(); m_adapter3 = nullptr; }
    if (m_factory) { m_factory->Release(); m_factory = nullptr; }
}

/* ---------- кадры (DWM) ---------- */

void SystemMonitor::pollFast() {
    DWM_TIMING_INFO ti{};
    ti.cbSize = sizeof(ti);
    float fps = m_v[(int)SensorId::FPS];

    if (SUCCEEDED(DwmGetCompositionTimingInfo(nullptr, &ti)) && ti.qpcCompose > 0) {
        if (m_lastCompose > 0 && ti.qpcCompose != m_lastCompose) {
            double dt = double(ti.qpcCompose - m_lastCompose) / double(m_freq.QuadPart);
            if (dt > 0.0009 && dt < 0.5)
                fps = (float)(1.0 / dt);
        }
        m_lastCompose = ti.qpcCompose;
    }
    clampf(fps, 0.f, 1000.f);

    m_v[(int)SensorId::FPS] = fps;
    m_v[(int)SensorId::FPSNative] = fps;
    m_v[(int)SensorId::Frametime] = fps > 0.1f ? 1000.f / fps : 0.f;

    // окно для AVG и 1% LOW
    if (fps > 0.1f) {
        m_fpsWin[m_fpsWinIdx] = fps;
        m_fpsWinIdx = (m_fpsWinIdx + 1) % 120;
        if (m_fpsWinLen < 120) m_fpsWinLen++;

        float sum = 0;
        std::vector<float> sorted;
        sorted.reserve(m_fpsWinLen);
        for (int i = 0; i < m_fpsWinLen; i++) { sum += m_fpsWin[i]; sorted.push_back(m_fpsWin[i]); }
        std::sort(sorted.begin(), sorted.end());
        if (m_fpsWinLen > 0) {
            m_v[(int)SensorId::FPSAvg] = sum / m_fpsWinLen;
            size_t lowIdx = (size_t)((sorted.size() - 1) * 0.01f);
            m_v[(int)SensorId::FPS1Low] = sorted[lowIdx];
        }
    }
}

void SystemMonitor::resetStats() {
    m_fpsWinLen = m_fpsWinIdx = 0;
    m_v[(int)SensorId::FPSAvg] = m_v[(int)SensorId::FPS1Low] = 0;
}

/* ---------- системные датчики ---------- */

void SystemMonitor::pollThermal() {
    if (!m_hasTemp) return;
    double k = pdhMaxArray(m_cThermal); // Кельвины
    double c = k - 273.15;
    if (c > 5.0 && c < 130.0) {
        m_v[(int)SensorId::CPUTemp] = (float)c;
        m_v[(int)SensorId::GPUTemp] = (float)c; // системная зона; per-GPU — только с vendor SDK
    }
}

void SystemMonitor::pollSys() {
    // ── CPU usage ──
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        uint64_t dIdle = fileTime64(idle) - fileTime64(m_prevIdle);
        uint64_t dKernel = fileTime64(kernel) - fileTime64(m_prevKernel);
        uint64_t dUser = fileTime64(user) - fileTime64(m_prevUser);
        uint64_t total = dKernel + dUser;
        if (total > 0) {
            float u = (1.f - (float)dIdle / (float)total) * 100.f;
            clampf(u, 0.f, 100.f);
            m_v[(int)SensorId::CPUUsage] = u;
        }
        m_prevIdle = idle; m_prevKernel = kernel; m_prevUser = user;
    }

    // ── CPU clock (~MHz из HAL) ──
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD mhz = 0, sz = sizeof(mhz);
        if (RegQueryValueExA(hKey, "~MHz", nullptr, nullptr, (LPBYTE)&mhz, &sz) == ERROR_SUCCESS)
            m_v[(int)SensorId::CPUClock] = (float)mhz;
        RegCloseKey(hKey);
    }

    // ── RAM ──
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        m_v[(int)SensorId::RAMUsage] = (float)ms.dwMemoryLoad;
        m_v[(int)SensorId::RAMUsed] = (float)((ms.ullTotalPhys - ms.ullAvailPhys) / (1024 * 1024));
        m_v[(int)SensorId::RAMFree] = (float)(ms.ullAvailPhys / (1024 * 1024));
    }

    // ── VRAM (DXGI 1.4) ──
    if (m_adapter3 && m_vramTotalMB > 0) {
        DXGI_QUERY_VIDEO_MEMORY_INFO mi{};
        if (SUCCEEDED(m_adapter3->QueryVideoMemoryInfo(
                0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &mi))) {
            float used = (float)(mi.CurrentUsage / (1024 * 1024));
            clampf(used, 0.f, m_vramTotalMB);
            m_v[(int)SensorId::VRAM] = used;
            m_v[(int)SensorId::VRAMPct] = used / m_vramTotalMB * 100.f;
        }
    }

    // ── PDH ──
    if (m_pdhReady) {
        PdhCollectQueryData(m_q);
        m_v[(int)SensorId::GPUUsage] = (float)pdhMaxArray(m_cGpu);
        m_v[(int)SensorId::DiskRead] = (float)(pdhScalar(m_cDiskR) / (1024.0 * 1024.0));
        m_v[(int)SensorId::DiskWrite] = (float)(pdhScalar(m_cDiskW) / (1024.0 * 1024.0));
        double dt = pdhScalar(m_cDiskT);
        if (dt > 100.0) dt = 100.0; // % Disk Time может превышать 100 на виртуалках
        m_v[(int)SensorId::DiskActive] = (float)dt;
        m_v[(int)SensorId::NetUp] = (float)(pdhSumArray(m_cNetS) / 1024.0);
        m_v[(int)SensorId::NetDown] = (float)(pdhSumArray(m_cNetR) / 1024.0);
        pollThermal();
    }
}

/* ---------- история ---------- */

void SystemMonitor::pushHistory() {
    for (int s = 0; s < SENSOR_COUNT; s++)
        m_hist[s][m_histIdx] = m_v[s];
    m_histIdx = (m_histIdx + 1) % HISTORY_SIZE;
    if (m_histLen < HISTORY_SIZE) m_histLen++;
}
