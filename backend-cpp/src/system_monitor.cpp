#include "system_monitor.hpp"
#include <thread>
#include <chrono>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#include <powerbase.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#endif

namespace pc_monitor {

class SystemMonitor::Impl {
public:
#ifdef _WIN32
    PDH_HQUERY cpuQuery = nullptr;
    PDH_HCOUNTER cpuTotal = nullptr;
    std::vector<PDH_HCOUNTER> cpuCores;
    bool initialized = false;
#endif

    Impl() = default;
    
    ~Impl() {
#ifdef _WIN32
        cleanup();
#endif
    }
    
    Result<void> initialize() {
#ifdef _WIN32
        // Initialize PDH for CPU monitoring
        if (PdhOpenQuery(nullptr, 0, &cpuQuery) != ERROR_SUCCESS) {
            return std::unexpected(SystemError::INITIALIZATION_FAILED);
        }
        
        // Add CPU total counter
        if (PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &cpuTotal) != ERROR_SUCCESS) {
            cleanup();
            return std::unexpected(SystemError::INITIALIZATION_FAILED);
        }
        
        // Get number of logical processors
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        // Add individual core counters
        cpuCores.reserve(sysInfo.dwNumberOfProcessors);
        for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; ++i) {
            PDH_HCOUNTER coreCounter;
            std::wstring counterPath = std::format(L"\\Processor({})\\% Processor Time", i);
            
            if (PdhAddEnglishCounter(cpuQuery, counterPath.c_str(), 0, &coreCounter) == ERROR_SUCCESS) {
                cpuCores.push_back(coreCounter);
            }
        }
        
        // Collect first sample (required for PDH)
        PdhCollectQueryData(cpuQuery);
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        
        initialized = true;
        return {};
#else
        return std::unexpected(SystemError::SYSTEM_ERROR);
#endif
    }
    
    Result<SystemStats> getCurrentStats() {
        if (!initialized) {
            return std::unexpected(SystemError::INITIALIZATION_FAILED);
        }
        
        SystemStats stats;
        stats.timestamp = std::chrono::system_clock::now();
        
        auto cpuResult = getCPUStats();
        if (!cpuResult) {
            return std::unexpected(cpuResult.error());
        }
        stats.cpu = std::move(*cpuResult);
        
        auto memResult = getMemoryStats();
        if (!memResult) {
            return std::unexpected(memResult.error());
        }
        stats.memory = std::move(*memResult);
        
        return stats;
    }

private:
    void cleanup() {
#ifdef _WIN32
        if (cpuQuery) {
            PdhCloseQuery(cpuQuery);
            cpuQuery = nullptr;
        }
        cpuCores.clear();
        initialized = false;
#endif
    }
    
    Result<CPUUsageData> getCPUStats() {
#ifdef _WIN32
        // Collect query data
        if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS) {
            return std::unexpected(SystemError::DATA_UNAVAILABLE);
        }
        
        CPUUsageData cpuData;
        
        // Get total CPU usage
        PDH_FMT_COUNTERVALUE counterVal;
        if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
            cpuData.overall = std::clamp(counterVal.dblValue, 0.0, 100.0);
        }
        
        // Get individual core usage
        cpuData.cores.reserve(cpuCores.size());
        for (std::size_t i = 0; i < cpuCores.size(); ++i) {
            if (PdhGetFormattedCounterValue(cpuCores[i], PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
                CPUCoreData coreData{
                    .coreId = static_cast<std::uint32_t>(i),
                    .usage = std::clamp(counterVal.dblValue, 0.0, 100.0),
                    .frequency = getCoreFrequency(i)
                };
                cpuData.cores.push_back(std::move(coreData));
            }
        }
        
        // Calculate average frequency
        if (!cpuData.cores.empty()) {
            auto frequencies = cpuData.cores | std::views::transform([](const auto& core) { 
                return core.frequency; 
            });
            cpuData.averageFrequency = static_cast<std::uint64_t>(utils::average(frequencies));
        }
        
        // Try to get CPU temperature (optional)
        cpuData.temperature = getCPUTemperature();
        
        return cpuData;
#else
        return std::unexpected(SystemError::SYSTEM_ERROR);
#endif
    }
    
    Result<MemoryUsageData> getMemoryStats() {
#ifdef _WIN32
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        
        if (!GlobalMemoryStatusEx(&memStatus)) {
            return std::unexpected(SystemError::DATA_UNAVAILABLE);
        }
        
        // Get more detailed memory info
        PROCESS_MEMORY_COUNTERS_EX pmc;
        HANDLE hProcess = GetCurrentProcess();
        
        MemoryUsageData memData{
            .total = memStatus.ullTotalPhys,
            .used = memStatus.ullTotalPhys - memStatus.ullAvailPhys,
            .available = memStatus.ullAvailPhys,
            .cache = 0, // Windows doesn't easily expose cache info
            .buffers = 0, // Windows doesn't easily expose buffer info
            .usagePercent = static_cast<double>(memStatus.dwMemoryLoad)
        };
        
        // Try to get cache information from performance counters
        memData.cache = getCacheSize();
        
        return memData;
#else
        return std::unexpected(SystemError::SYSTEM_ERROR);
#endif
    }
    
#ifdef _WIN32
    std::uint64_t getCoreFrequency(std::size_t coreIndex) {
        // Try to get processor frequency - simplified implementation
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            
            DWORD mhz;
            DWORD size = sizeof(mhz);
            if (RegQueryValueEx(hKey, L"~MHz", nullptr, nullptr, 
                reinterpret_cast<LPBYTE>(&mhz), &size) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return static_cast<std::uint64_t>(mhz);
            }
            RegCloseKey(hKey);
        }
        
        return 2400; // Default fallback frequency
    }
    
    std::optional<double> getCPUTemperature() {
        // CPU temperature is not easily accessible on Windows without WMI
        // This would require additional libraries like WMI or hardware-specific APIs
        // For now, return nullopt
        return std::nullopt;
    }
    
    std::uint64_t getCacheSize() {
        // Get cache size from system info - simplified
        DWORD bufferSize = 0;
        GetLogicalProcessorInformation(nullptr, &bufferSize);
        
        if (bufferSize > 0) {
            std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
                bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
            
            if (GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
                std::uint64_t totalCache = 0;
                for (const auto& info : buffer) {
                    if (info.Relationship == RelationCache) {
                        totalCache += info.Cache.Size;
                    }
                }
                return totalCache;
            }
        }
        
        return 0;
    }
#endif
};

// SystemMonitor implementation
SystemMonitor::SystemMonitor() : pImpl(std::make_unique<Impl>()) {}

SystemMonitor::~SystemMonitor() = default;

Result<void> SystemMonitor::initialize() {
    return pImpl->initialize();
}

Result<SystemStats> SystemMonitor::getCurrentStats() {
    return pImpl->getCurrentStats();
}

SystemMonitor::StatsGenerator SystemMonitor::streamStats(std::chrono::milliseconds interval) {
    while (true) {
        auto stats = getCurrentStats();
        if (stats) {
            co_yield *stats;
        }
        
        std::this_thread::sleep_for(interval);
    }
}

} // namespace pc_monitor