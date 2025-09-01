import React, { useState } from 'react';
import { Monitor, Settings, Wifi, WifiOff } from 'lucide-react';
import { CPUMonitor } from './CPUMonitor';
import { MemoryMonitor } from './MemoryMonitor';
import { CPUCoreGrid } from './CPUCoreGrid';
import { useSystemStats } from '../hooks/useSystemStats';
import { useRealTimeChart } from '../hooks/useRealTimeChart';
import { TimeRange } from '../types';

interface DashboardProps {
  refreshInterval?: number;
  theme?: 'light' | 'dark';
}

export const Dashboard: React.FC<DashboardProps> = ({
  refreshInterval = 1000,
  theme = 'dark',
}) => {
  const [timeRange, setTimeRange] = useState<TimeRange>('1m');
  const { cpu, memory, cores, isConnected, error } = useSystemStats(refreshInterval);
  
  const cpuChartData = useRealTimeChart(
    cpu ? [{ timestamp: cpu.timestamp || Date.now(), value: cpu.overall }] : [],
    timeRange === '1m' ? 60 : timeRange === '5m' ? 300 : 3600
  );
  
  const memoryChartData = useRealTimeChart(
    memory ? [{ timestamp: memory.timestamp || Date.now(), value: memory.usagePercent }] : [],
    timeRange === '1m' ? 60 : timeRange === '5m' ? 300 : 3600
  );

  return (
    <div className="min-h-screen bg-bg-primary text-text-primary p-6">
      <header className="flex items-center justify-between mb-8">
        <div className="flex items-center gap-3">
          <Monitor className="w-8 h-8 text-blue-400" />
          <h1 className="text-2xl font-bold">PC Performance Monitor</h1>
        </div>
        
        <div className="flex items-center gap-4">
          <div className="flex items-center gap-2">
            {isConnected ? (
              <Wifi className="w-5 h-5 text-green-400" />
            ) : (
              <WifiOff className="w-5 h-5 text-red-400" />
            )}
            <span className="text-sm text-text-secondary">
              {isConnected ? 'Connected' : 'Disconnected'}
            </span>
          </div>
          
          <button className="p-2 hover:bg-bg-secondary rounded-lg transition-colors">
            <Settings className="w-5 h-5 text-text-secondary" />
          </button>
        </div>
      </header>

      {error && (
        <div className="bg-red-900/20 border border-red-600 rounded-lg p-4 mb-6">
          <div className="flex items-center gap-2 text-red-400">
            <span className="font-semibold">Connection Error:</span>
            <span>{error.message}</span>
          </div>
        </div>
      )}

      <div className="grid grid-cols-1 xl:grid-cols-2 gap-6 mb-6">
        {cpu && (
          <CPUMonitor
            data={cpuChartData.chartData}
            currentData={cpu}
            timeRange={timeRange}
            onTimeRangeChange={setTimeRange}
          />
        )}
        
        {memory && (
          <MemoryMonitor
            data={memoryChartData.chartData}
            currentData={memory}
            showBreakdown={true}
          />
        )}
      </div>

      {cores && cores.length > 0 && (
        <CPUCoreGrid
          cores={cores}
          layout="grid"
          showFrequency={true}
        />
      )}

      <footer className="mt-8 text-center text-sm text-text-secondary">
        <div>
          Last updated: {cpu?.timestamp ? new Date(cpu.timestamp).toLocaleTimeString('ko-KR') : 'N/A'}
        </div>
        <div className="mt-1">
          Refresh interval: {refreshInterval / 1000}s
        </div>
      </footer>
    </div>
  );
};