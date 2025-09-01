using System.Diagnostics;
using System.Management;
using System.Runtime.InteropServices;
using PCMonitor.Models;

namespace PCMonitor.Services;

public sealed class SystemMonitorService : ISystemMonitorService, IDisposable
{
    private readonly List<PerformanceCounter> _cpuCounters = [];
    private PerformanceCounter? _totalCpuCounter;
    private ManagementObjectSearcher? _cpuTemperatureSearcher;
    private bool _initialized;
    private readonly object _lockObject = new();

    [DllImport("kernel32.dll")]
    private static extern void GetSystemInfo(out SYSTEM_INFO lpSystemInfo);

    [DllImport("kernel32.dll")]
    private static extern bool GlobalMemoryStatusEx(ref MEMORY_STATUS_EX lpBuffer);

    [StructLayout(LayoutKind.Sequential)]
    private struct SYSTEM_INFO
    {
        public uint dwOemId;
        public uint dwPageSize;
        public IntPtr lpMinimumApplicationAddress;
        public IntPtr lpMaximumApplicationAddress;
        public UIntPtr dwActiveProcessorMask;
        public uint dwNumberOfProcessors;
        public uint dwProcessorType;
        public uint dwAllocationGranularity;
        public ushort dwProcessorLevel;
        public ushort dwProcessorRevision;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct MEMORY_STATUS_EX
    {
        public uint dwLength;
        public uint dwMemoryLoad;
        public ulong ullTotalPhys;
        public ulong ullAvailPhys;
        public ulong ullTotalPageFile;
        public ulong ullAvailPageFile;
        public ulong ullTotalVirtual;
        public ulong ullAvailVirtual;
        public ulong ullAvailExtendedVirtual;
    }

    public bool IsInitialized => _initialized;

    public async Task<bool> InitializeAsync(CancellationToken cancellationToken = default)
    {
        if (_initialized) return true;

        try
        {
            await Task.Run(() => InitializeCPUCounters(), cancellationToken);
            InitializeTemperatureMonitoring();
            await Task.Delay(1000, cancellationToken);
            _initialized = true;
            return true;
        }
        catch
        {
            return false;
        }
    }

    public async Task<CPUUsageData> GetCPUDataAsync()
    {
        if (!_initialized) throw new InvalidOperationException("Service not initialized");

        return await Task.Run(() =>
        {
            lock (_lockObject)
            {
                var overall = _totalCpuCounter?.NextValue() ?? 0.0f;
                var cores = new List<CPUCoreData>();
                var frequencies = new List<ulong>();

                for (int i = 0; i < _cpuCounters.Count; i++)
                {
                    var usage = _cpuCounters[i].NextValue();
                    var frequency = GetCoreFrequency(i);
                    frequencies.Add(frequency);

                    cores.Add(new CPUCoreData
                    {
                        CoreId = (uint)i,
                        Usage = Math.Clamp(usage, 0, 100),
                        Frequency = frequency
                    });
                }

                var averageFrequency = frequencies.Count > 0 ? (ulong)frequencies.Average(f => (double)f) : 0UL;

                return new CPUUsageData
                {
                    Overall = Math.Clamp(overall, 0, 100),
                    Temperature = GetCPUTemperature(),
                    AverageFrequency = averageFrequency,
                    Cores = cores.AsReadOnly()
                };
            }
        });
    }

    public async Task<MemoryUsageData> GetMemoryDataAsync()
    {
        if (!_initialized) throw new InvalidOperationException("Service not initialized");

        return await Task.Run(() =>
        {
            var memStatus = new MEMORY_STATUS_EX
            {
                dwLength = (uint)Marshal.SizeOf<MEMORY_STATUS_EX>()
            };

            if (!GlobalMemoryStatusEx(ref memStatus))
            {
                throw new InvalidOperationException("Failed to get memory status");
            }

            var used = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
            var (cache, buffers) = GetCacheAndBufferInfo();

            return new MemoryUsageData
            {
                Total = memStatus.ullTotalPhys,
                Used = used,
                Available = memStatus.ullAvailPhys,
                Cache = cache,
                Buffers = buffers,
                UsagePercent = memStatus.dwMemoryLoad
            };
        });
    }

    public async Task<SystemStats> GetSystemStatsAsync()
    {
        var cpuTask = GetCPUDataAsync();
        var memoryTask = GetMemoryDataAsync();

        await Task.WhenAll(cpuTask, memoryTask);

        return new SystemStats
        {
            CPU = await cpuTask,
            Memory = await memoryTask,
            Timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()
        };
    }

    private void InitializeCPUCounters()
    {
        _totalCpuCounter = new PerformanceCounter("Processor", "% Processor Time", "_Total", true);
        
        GetSystemInfo(out var sysInfo);
        var coreCount = (int)sysInfo.dwNumberOfProcessors;
        
        for (int i = 0; i < coreCount; i++)
        {
            try
            {
                var counter = new PerformanceCounter("Processor", "% Processor Time", i.ToString(), true);
                _cpuCounters.Add(counter);
            }
            catch
            {
                // Skip failed counters
            }
        }
    }

    private void InitializeTemperatureMonitoring()
    {
        try
        {
            _cpuTemperatureSearcher = new ManagementObjectSearcher(
                "root\\WMI", 
                "SELECT * FROM MSAcpi_ThermalZoneTemperature");
        }
        catch
        {
            // Temperature monitoring not available
        }
    }

    private static ulong GetCoreFrequency(int coreIndex)
    {
        try
        {
            using var counter = new PerformanceCounter("Processor Information", "Processor Frequency", $"0,{coreIndex}", true);
            return (ulong)counter.NextValue();
        }
        catch
        {
            return 2400UL;
        }
    }

    private double? GetCPUTemperature()
    {
        try
        {
            if (_cpuTemperatureSearcher == null) return null;

            using var collection = _cpuTemperatureSearcher.Get();
            foreach (ManagementObject obj in collection)
            {
                if (obj["CurrentTemperature"] is uint temp)
                {
                    return (temp / 10.0) - 273.15;
                }
            }
        }
        catch
        {
            // Temperature not available
        }
        
        return null;
    }

    private static (ulong cache, ulong buffers) GetCacheAndBufferInfo()
    {
        try
        {
            using var cacheCounter = new PerformanceCounter("Memory", "Cache Bytes", true);
            using var modifiedCounter = new PerformanceCounter("Memory", "Modified Page List Bytes", true);
            
            var cache = (ulong)cacheCounter.NextValue();
            var buffers = (ulong)modifiedCounter.NextValue();
            
            return (cache, buffers);
        }
        catch
        {
            return (0, 0);
        }
    }

    public void Dispose()
    {
        _totalCpuCounter?.Dispose();
        
        foreach (var counter in _cpuCounters)
        {
            counter?.Dispose();
        }
        _cpuCounters.Clear();
        
        _cpuTemperatureSearcher?.Dispose();
    }
}