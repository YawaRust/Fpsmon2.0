#pragma once
#include "../common.h"
#include "../config/Model.h"

/*
 * SystemMonitor: опрос реальных датчиков Windows.
 *
 *  CPU usage     — GetSystemTimes (дельта busy/total)
 *  CPU clock     — HKLM\...\CentralProcessor\0 "~MHz"
 *  RAM           — GlobalMemoryStatusEx
 *  GPU usage     — PDH "\GPU Engine(*engtype_3D)\Utilization Percentage" (max)
 *  VRAM          — IDXGIAdapter3::QueryVideoMemoryInfo
 *  Температуры   — PDH "\Thermal Zone Information(*)\Temperature" (если есть)
 *  Диск / сеть   — PDH PhysicalDisk(_Total) / Network Interface(*)
 *  Кадры         — DwmGetCompositionTimingInfo (композиторный frame pacing)
 *
 * PWR / FAN / частота GPU требуют NVAPI / ADL / RAPL — датчики помечены
 * недоступными и скрываются рендерером до появления соответствующего модуля.
 */

class SystemMonitor {
public:
    bool init(int gpuIndex = 0);
    void shutdown();

    void pollFast();   // кадры (fpsIntervalMs)
    void pollSys();     // железо (updateIntervalMs)
    void pushHistory(); // запись в ring-буферы (фикс 10 Гц)
    void resetStats();

    int gpuCount() const { return (int)m_gpuNames.size(); }
    const std::vector<std::wstring>& gpuNames() const { return m_gpuNames; }
    int selectedGpu() const { return m_selectedGpu; }
    float vramTotal() const { return m_vramTotalMB; }
    bool hasGpu() const { return !m_gpuNames.empty(); }
    void setSelectedGpu(int idx);

    float value(SensorId id) const { return m_v[(int)id]; }
    const float* hist(SensorId id) const { return m_hist[(int)id]; }
    int histSize() const { return m_histLen; }
    int histHead() const { return m_histIdx; }

private:
    double pdhMaxArrayFiltered(PDH_HCOUNTER c, const std::string& filter);
    double pdhSumArray(PDH_HCOUNTER c);
    double pdhScalar(PDH_HCOUNTER c);

    float m_v[SENSOR_COUNT] = {};
    float m_hist[SENSOR_COUNT][HISTORY_SIZE] = {};
    int m_histIdx = 0;
    int m_histLen = 0;

    // CPU по GetSystemTimes
    FILETIME m_prevIdle{}, m_prevKernel{}, m_prevUser{};

    // PDH
    PDH_HQUERY m_q = nullptr;
    PDH_HCOUNTER m_cGpu = nullptr;
    PDH_HCOUNTER m_cDiskR = nullptr, m_cDiskW = nullptr, m_cDiskT = nullptr;
    PDH_HCOUNTER m_cNetS = nullptr, m_cNetR = nullptr;
    PDH_HCOUNTER m_cCpuFreq = nullptr;
    bool m_pdhReady = false;

    // DXGI VRAM
    IDXGIFactory1* m_factory = nullptr;
    IDXGIAdapter3* m_adapter3 = nullptr;
    float m_vramTotalMB = 0.f;

    std::vector<IDXGIAdapter1*> m_adapters;
    std::vector<std::wstring> m_gpuNames;
    std::vector<LUID> m_gpuLuids;
    int m_selectedGpu = 0;
    std::string m_gpuFilter;

    // кадры через DWM
    LARGE_INTEGER m_freq{};
    UINT64 m_lastCompose = 0;
    float m_fpsWin[120];
    int m_fpsWinLen = 0;
    int m_fpsWinIdx = 0;
};
