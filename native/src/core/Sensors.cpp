#include "Sensors.h"
#include "../utils/Logger.h"
#include <powrprof.h>

struct CpuPowerInfo {
    unsigned long Number;
    unsigned long MaxMhz;
    unsigned long CurrentMhz;
    unsigned long MhzLimit;
    unsigned long MaxIdleState;
    unsigned long CurrentIdleState;
};

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

double SystemMonitor::pdhMaxArrayFiltered(PDH_HCOUNTER c, const std::string& filter) {
    if (!c || filter.empty()) return 0.0;
    DWORD size = 0, count = 0;
    if (PdhGetFormattedCounterArrayA(c, PDH_FMT_DOUBLE, &size, &count, nullptr)
        != PDH_MORE_DATA || size == 0)
        return 0.0;
    std::vector<BYTE> buf(size);
    auto* arr = (PDH_FMT_COUNTERVALUE_ITEM_A*)buf.data();
    if (PdhGetFormattedCounterArrayA(c, PDH_FMT_DOUBLE, &size, &count, arr) != ERROR_SUCCESS)
        return 0.0;
    double mx = 0.0;
    for (DWORD i = 0; i < count; i++) {
        if (arr[i].FmtValue.CStatus != ERROR_SUCCESS) continue;
        std::string name = arr[i].szName ? arr[i].szName : "";
        if (name.find(filter) == std::string::npos) continue;
        if (arr[i].FmtValue.doubleValue > mx) mx = arr[i].FmtValue.doubleValue;
    }
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

bool SystemMonitor::init(int gpuIndex) {
    QueryPerformanceFrequency(&m_freq);
    GetSystemTimes(&m_prevIdle, &m_prevKernel, &m_prevUser);

    // DXGI 1.4 → перечисление адаптеров (GPU)
    IDXGIFactory1* fac1 = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&fac1)))) {
        m_factory = fac1;
        for (UINT i = 0; ; i++) {
            IDXGIAdapter1* ad = nullptr;
            if (FAILED(fac1->EnumAdapters1(i, &ad))) break;
            DXGI_ADAPTER_DESC1 desc{};
            if (FAILED(ad->GetDesc1(&desc)) || (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                ad->Release();
                continue;
            }
            m_adapters.push_back(ad);
            m_gpuNames.push_back(desc.Description);
            m_gpuLuids.push_back(desc.AdapterLuid);
        }
    }
    // Выбираем адаптер с наибольшим объёмом видеопамяти по умолчанию
    int best = 0;
    float bestMem = -1.f;
    for (int i = 0; i < (int)m_adapters.size(); i++) {
        if (!m_adapters[i]) continue;
        DXGI_ADAPTER_DESC1 d{};
        if (SUCCEEDED(m_adapters[i]->GetDesc1(&d))) {
            float mem = (float)(d.DedicatedVideoMemory / (1024 * 1024));
            if (mem > bestMem) { bestMem = mem; best = i; }
        }
    }
    if (gpuIndex < 0 || gpuIndex >= (int)m_gpuNames.size()) gpuIndex = best;
    setSelectedGpu(gpuIndex);
    LOGF("DXGI: адаптеров %d, выбран %d, VRAM total %.0f MB",
         (int)m_gpuNames.size(), m_selectedGpu, m_vramTotalMB);

    // PDH
    if (PdhOpenQueryA(nullptr, 0, &m_q) != ERROR_SUCCESS) {
        LOG("PDH: не удалось открыть запрос");
    } else {
        PdhAddEnglishCounterA(m_q, "\\Processor(_Total)\\% Processor Time", 0, nullptr); // Dummy for wakeup
        PdhAddEnglishCounterA(m_q, "\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &m_cGpu);
        PdhAddEnglishCounterA(m_q, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &m_cDiskR);
        PdhAddEnglishCounterA(m_q, "\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &m_cDiskW);
        PdhAddEnglishCounterA(m_q, "\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_cDiskT);
        PdhAddEnglishCounterA(m_q, "\\Network Interface(*)\\Bytes Sent/sec", 0, &m_cNetS);
        PdhAddEnglishCounterA(m_q, "\\Network Interface(*)\\Bytes Received/sec", 0, &m_cNetR);
        PdhAddEnglishCounterA(m_q, "\\Processor Information(_Total)\\Processor Frequency", 0, &m_cCpuFreq);

        PdhCollectQueryData(m_q);
        m_pdhReady = true;
        LOG("PDH: Запрос открыт");
    }

    if (m_vramTotalMB > 0)
        m_v[(int)SensorId::VRAMPct] = 0;

    LOG("SystemMonitor инициализирован");
    return true;
}

void SystemMonitor::shutdown() {
    if (m_q) { PdhCloseQuery(m_q); m_q = nullptr; }
    if (m_adapter3) { m_adapter3->Release(); m_adapter3 = nullptr; }
    for (auto* a : m_adapters) if (a) a->Release();
    m_adapters.clear();
    m_gpuNames.clear();
    m_gpuLuids.clear();
    if (m_factory) { m_factory->Release(); m_factory = nullptr; }
}


void SystemMonitor::setSelectedGpu(int idx) {
    if (m_gpuNames.empty()) return;
    if (idx < 0 || idx >= (int)m_gpuNames.size()) idx = 0;
    m_selectedGpu = idx;
    if (m_adapter3) { m_adapter3->Release(); m_adapter3 = nullptr; }
    m_vramTotalMB = 0.f;
    if (idx < (int)m_adapters.size() && m_adapters[idx]) {
        IDXGIAdapter3* a3 = nullptr;
        if (SUCCEEDED(m_adapters[idx]->QueryInterface(IID_PPV_ARGS(&a3)))) {
            m_adapter3 = a3;
            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(m_adapters[idx]->GetDesc1(&desc))) {
                m_vramTotalMB = (float)(desc.DedicatedVideoMemory / (1024 * 1024));
                char buf[64];
                sprintf_s(buf, "luid_0x%08x_0x%08x",
                          desc.AdapterLuid.LowPart, desc.AdapterLuid.HighPart);
                m_gpuFilter = buf;
            }
        }
    }
    if (m_vramTotalMB > 0)
        m_v[(int)SensorId::VRAMPct] = 0;
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

    // окно для AVG и 1% LOW (используем 120 сэмплов ~6 секунд при 20Гц)
    if (fps > 0.1f) {
        m_fpsWin[m_fpsWinIdx] = fps;
        m_fpsWinIdx = (m_fpsWinIdx + 1) % 120;
        if (m_fpsWinLen < 120) m_fpsWinLen++;

        if (m_fpsWinLen > 10) {
            float sum = 0;
            float sorted[120];
            for (int i = 0; i < m_fpsWinLen; i++) {
                sum += m_fpsWin[i];
                sorted[i] = m_fpsWin[i];
            }
            std::sort(sorted, sorted + m_fpsWinLen);
            
            m_v[(int)SensorId::FPSAvg] = sum / (float)m_fpsWinLen;
            // 1% Low — это среднее значение нижнего 1% выборки или просто 1-й процентиль
            int lowIdx = (int)((float)m_fpsWinLen * 0.01f);
            if (lowIdx < 0) lowIdx = 0;
            m_v[(int)SensorId::FPS1Low] = sorted[lowIdx];
        }
    }
}

void SystemMonitor::resetStats() {
    m_fpsWinLen = m_fpsWinIdx = 0;
    m_v[(int)SensorId::FPSAvg] = m_v[(int)SensorId::FPS1Low] = 0;
}

/* ---------- системные датчики ---------- */

void SystemMonitor::pollSys() {
    if (m_pdhReady) {
        PdhCollectQueryData(m_q);
    }
    
    // ── CPU usage ──
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        uint64_t dIdle = fileTime64(idle) - fileTime64(m_prevIdle);
        uint64_t dKernel = fileTime64(kernel) - fileTime64(m_prevKernel);
        uint64_t dUser = fileTime64(user) - fileTime64(m_prevUser);
        uint64_t total = dKernel + dUser;
        if (total > 0) {
            float u = (1.0f - (float)dIdle / (float)total) * 100.0f;
            if (u < 0) u = 0; if (u > 100) u = 100;
            m_v[(int)SensorId::CPUUsage] = u;
        }
        m_prevIdle = idle; m_prevKernel = kernel; m_prevUser = user;
    }

    // ── CPU clock (текущая частота с бустом, как в Диспетчере задач) ──
    {
        if (m_cCpuFreq) {
            // pollSys collected all PDH counters at function entry; read that sample here.
            double f = pdhScalar(m_cCpuFreq);
            if (f > 0) m_v[(int)SensorId::CPUClock] = (float)f;
        }
        if (m_v[(int)SensorId::CPUClock] <= 0) {
            SYSTEM_INFO si{}; GetSystemInfo(&si);
            DWORD n = si.dwNumberOfProcessors > 0 ? si.dwNumberOfProcessors : 1;
            std::vector<CpuPowerInfo> info(n);
            if (CallNtPowerInformation(ProcessorInformation, nullptr, 0,
                    info.data(), (ULONG)(n * sizeof(CpuPowerInfo))) == 0) {
                DWORD mx = 0;
                for (DWORD i = 0; i < n; i++)
                    if (info[i].CurrentMhz > mx) mx = info[i].CurrentMhz;
                if (mx > 0) m_v[(int)SensorId::CPUClock] = (float)mx;
            }
        }
        if (m_v[(int)SensorId::CPUClock] <= 0) {
            HKEY hKey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD mhz = 0, sz = sizeof(mhz);
                if (RegQueryValueExA(hKey, "~MHz", nullptr, nullptr, (LPBYTE)&mhz, &sz) == ERROR_SUCCESS)
                    m_v[(int)SensorId::CPUClock] = (float)mhz;
                RegCloseKey(hKey);
            }
        }
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
        m_v[(int)SensorId::GPUUsage] = (float)pdhMaxArrayFiltered(m_cGpu, m_gpuFilter);
        m_v[(int)SensorId::DiskRead] = (float)(pdhScalar(m_cDiskR) / (1024.0 * 1024.0));
        m_v[(int)SensorId::DiskWrite] = (float)(pdhScalar(m_cDiskW) / (1024.0 * 1024.0));
        double dt = pdhScalar(m_cDiskT);
        if (dt > 100.0) dt = 100.0; // % Disk Time может превышать 100 на виртуалках
        m_v[(int)SensorId::DiskActive] = (float)dt;
        m_v[(int)SensorId::NetUp] = (float)(pdhSumArray(m_cNetS) / 1024.0);
        m_v[(int)SensorId::NetDown] = (float)(pdhSumArray(m_cNetR) / 1024.0);
    }
}

/* ---------- история ---------- */

void SystemMonitor::pushHistory() {
    for (int s = 0; s < SENSOR_COUNT; s++)
        m_hist[s][m_histIdx] = m_v[s];
    m_histIdx = (m_histIdx + 1) % HISTORY_SIZE;
    if (m_histLen < HISTORY_SIZE) m_histLen++;
}
