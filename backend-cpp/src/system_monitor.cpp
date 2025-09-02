#include "system_monitor.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>

#include <pdh.h>
#include <powerbase.h>
#include <psapi.h>
#include <windows.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")

namespace pc_monitor {

class SystemMonitor::Impl {
public:
    PDH_HQUERY cpuQuery = nullptr;
    PDH_HCOUNTER cpuTotal = nullptr;
    std::vector<PDH_HCOUNTER> cpuCores;
    bool initialized = false;

    Impl() = default;

    ~Impl() {
        Cleanup();
    }

    Result<void> Initialize() {
        // Initialize PDH for CPU monitoring
        if (PdhOpenQuery(nullptr, 0, &cpuQuery) != ERROR_SUCCESS) {
            return std::unexpected(SystemError::INITIALIZATION_FAILED);
        }

        // Add CPU total counter
        if (PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &cpuTotal) != ERROR_SUCCESS) {
            Cleanup();
            return std::unexpected(SystemError::INITIALIZATION_FAILED);
        }

        // Get number of logical processors
        SYSTEM_INFO SysInfo;
        GetSystemInfo(&SysInfo);

        // Add individual core counters
        cpuCores.reserve(SysInfo.dwNumberOfProcessors);
        for (DWORD I = 0; I < SysInfo.dwNumberOfProcessors; ++I) {
            PDH_HCOUNTER CoreCounter = nullptr;
            std::wstring const CounterPath = std::format(L"\\Processor({})\\% Processor Time", I);

            if (PdhAddEnglishCounterW(cpuQuery, CounterPath.c_str(), 0, &CoreCounter) == ERROR_SUCCESS) {
                cpuCores.push_back(CoreCounter);
            }
        }

        // Collect first sample (required for PDH)
        PdhCollectQueryData(cpuQuery);
        std::this_thread::sleep_for(std::chrono::milliseconds{100});

        initialized = true;
        return {};
    }

    Result<SystemStats> GetCurrentStats() {
        if (!initialized) {
            return std::unexpected(SystemError::INITIALIZATION_FAILED);
        }

        SystemStats Stats;
        Stats.timestamp = std::chrono::system_clock::now();

        auto CpuResult = GetCpuStats();
        if (!CpuResult) {
            return std::unexpected(CpuResult.error());
        }
        Stats.cpu = std::move(*CpuResult);

        auto MemResult = GetMemoryStats();
        if (!MemResult) {
            return std::unexpected(MemResult.error());
        }
        Stats.memory = *MemResult;

        return Stats;
    }

private:
    void Cleanup() {
        if (cpuQuery != nullptr) {
            PdhCloseQuery(cpuQuery);
            cpuQuery = nullptr;
        }
        cpuCores.clear();
        initialized = false;
    }

    Result<CPUUsageData> GetCpuStats() {
        // Collect query data
        if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS) {
            return std::unexpected(SystemError::DATA_UNAVAILABLE);
        }

        CPUUsageData CpuData;

        // Get total CPU usage
        PDH_FMT_COUNTERVALUE CounterVal;
        if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, nullptr, &CounterVal) == ERROR_SUCCESS) {
            CpuData.overall = std::clamp(CounterVal.doubleValue, 0.0, 100.0);
        }

        // Get individual core usage
        CpuData.cores.reserve(cpuCores.size());
        for (std::size_t I = 0; I < cpuCores.size(); ++I) {
            if (PdhGetFormattedCounterValue(cpuCores[I], PDH_FMT_DOUBLE, nullptr, &CounterVal) == ERROR_SUCCESS) {
                CPUCoreData CoreData{.coreId = static_cast<std::uint32_t>(I),
                                     .usage = std::clamp(CounterVal.doubleValue, 0.0, 100.0),
                                     .frequency = GetCoreFrequency(I)};
                CpuData.cores.push_back(std::move(CoreData));
            }
        }

        // Calculate average frequency
        if (!CpuData.cores.empty()) {
            auto Frequencies = CpuData.cores | std::views::transform([](const auto& core) { return core.frequency; });
            CpuData.averageFrequency = static_cast<std::uint64_t>(utils::Average(Frequencies));
        }

        // Try to get CPU temperature (optional)
        CpuData.temperature = GetCpuTemperature();

        return CpuData;
    }

    Result<MemoryUsageData> GetMemoryStats() {
        MEMORYSTATUSEX MemStatus;
        MemStatus.dwLength = sizeof(MemStatus);

        if (GlobalMemoryStatusEx(&MemStatus) == 0) {
            return std::unexpected(SystemError::DATA_UNAVAILABLE);
        }

        // Get more detailed memory info
        // PROCESS_MEMORY_COUNTERS_EX pmc;  // Reserved for future use
        // HANDLE hProcess = GetCurrentProcess();  // Reserved for future use

        MemoryUsageData MemData{.total = MemStatus.ullTotalPhys,
                                .used = MemStatus.ullTotalPhys - MemStatus.ullAvailPhys,
                                .available = MemStatus.ullAvailPhys,
                                .cache = 0,    // Windows doesn't easily expose cache info
                                .buffers = 0,  // Windows doesn't easily expose buffer info
                                .usagePercent = static_cast<double>(MemStatus.dwMemoryLoad)};

        // Try to get cache information from performance counters
        MemData.cache = GetCacheSize();

        return MemData;
    }

    static std::uint64_t GetCoreFrequency(std::size_t /* coreIndex */) {
        // Try to get processor frequency - simplified implementation
        HKEY HKey = nullptr;
        if (RegOpenKeyExW(
                HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &HKey) ==
            ERROR_SUCCESS) {
            DWORD Mhz = 0;
            DWORD Size = sizeof(Mhz);
            if (RegQueryValueExW(HKey, L"~MHz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&Mhz), &Size) ==
                ERROR_SUCCESS) {
                RegCloseKey(HKey);
                return static_cast<std::uint64_t>(Mhz);
            }
            RegCloseKey(HKey);
        }

        return 2400;  // Default fallback frequency
    }

    static std::optional<double> GetCpuTemperature() {
        // CPU temperature is not easily accessible on Windows without WMI
        // This would require additional libraries like WMI or hardware-specific APIs
        // For now, return nullopt
        return std::nullopt;
    }

    static std::uint64_t GetCacheSize() {
        // Get cache size from system info - simplified
        DWORD BufferSize = 0;
        GetLogicalProcessorInformation(nullptr, &BufferSize);

        if (BufferSize > 0) {
            std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> Buffer(BufferSize /
                                                                     sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));

            if (GetLogicalProcessorInformation(Buffer.data(), &BufferSize) != 0) {
                std::uint64_t TotalCache = 0;
                for (const auto& Info : Buffer) {
                    if (Info.Relationship == RelationCache) {
                        TotalCache += Info.Cache.Size;
                    }
                }
                return TotalCache;
            }
        }

        return 0;
    }
};

// SystemMonitor implementation
SystemMonitor::SystemMonitor() : pImpl_(std::make_unique<Impl>()) {}

SystemMonitor::~SystemMonitor() = default;

Result<void> SystemMonitor::Initialize() {
    return pImpl_->Initialize();
}

Result<SystemStats> SystemMonitor::GetCurrentStats() {
    return pImpl_->GetCurrentStats();
}

SystemMonitor::StatsGenerator SystemMonitor::StreamStats(std::chrono::milliseconds interval) {
    while (true) {
        auto Stats = GetCurrentStats();
        if (Stats) {
            co_yield *Stats;
        }

        std::this_thread::sleep_for(interval);
    }
}

}  // namespace pc_monitor