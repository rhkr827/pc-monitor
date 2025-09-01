import * as si from 'systeminformation';
import { CpuData, MemoryData, SystemStats, CoreData } from './types';

export class SystemMonitor {
  private updateInterval: NodeJS.Timeout | null = null;
  private listeners: Array<(stats: SystemStats) => void> = [];

  async getCpuData(): Promise<CpuData> {
    const [cpuLoad, cpuFreq, cpuTemp] = await Promise.all([
      si.currentLoad(),
      si.cpuCurrentSpeed(),
      si.cpuTemperature().catch(() => ({ main: null }))
    ]);

    const cores: CoreData[] = cpuLoad.cpus.map((core, index) => ({
      coreId: index,
      usage: Math.round(core.load * 100) / 100,
      frequency: cpuFreq.cores?.[index] || cpuFreq.avg
    }));

    return {
      overall: Math.round(cpuLoad.currentLoad * 100) / 100,
      temperature: cpuTemp.main || undefined,
      averageFrequency: cpuFreq.avg,
      cores
    };
  }

  async getMemoryData(): Promise<MemoryData> {
    const memory = await si.mem();
    
    return {
      total: memory.total,
      used: memory.used,
      available: memory.available,
      cache: memory.cached,
      buffers: memory.buffcache,
      usagePercent: Math.round((memory.used / memory.total) * 10000) / 100
    };
  }

  async getSystemStats(): Promise<SystemStats> {
    const [cpu, memory] = await Promise.all([
      this.getCpuData(),
      this.getMemoryData()
    ]);

    return {
      cpu,
      memory,
      timestamp: Date.now()
    };
  }

  startMonitoring(intervalMs: number = 1000): void {
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
    }

    this.updateInterval = setInterval(async () => {
      try {
        const stats = await this.getSystemStats();
        this.listeners.forEach(listener => listener(stats));
      } catch (error) {
        console.error('Error getting system stats:', error);
      }
    }, intervalMs);
  }

  stopMonitoring(): void {
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
      this.updateInterval = null;
    }
  }

  addListener(listener: (stats: SystemStats) => void): void {
    this.listeners.push(listener);
  }

  removeListener(listener: (stats: SystemStats) => void): void {
    const index = this.listeners.indexOf(listener);
    if (index > -1) {
      this.listeners.splice(index, 1);
    }
  }

  async initialize(): Promise<boolean> {
    try {
      // Test if we can get system information
      await this.getSystemStats();
      return true;
    } catch (error) {
      console.error('Failed to initialize system monitor:', error);
      return false;
    }
  }
}