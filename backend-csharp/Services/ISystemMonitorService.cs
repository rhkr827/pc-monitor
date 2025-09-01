using PCMonitor.Models;

namespace PCMonitor.Services;

public interface ISystemMonitorService
{
    Task<bool> InitializeAsync(CancellationToken cancellationToken = default);
    Task<CPUUsageData> GetCPUDataAsync();
    Task<MemoryUsageData> GetMemoryDataAsync();
    Task<SystemStats> GetSystemStatsAsync();
    bool IsInitialized { get; }
}