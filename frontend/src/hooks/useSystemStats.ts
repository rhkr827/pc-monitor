import { useState, useEffect, useRef } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { SystemStats, CPUUsageData, MemoryUsageData, CPUCoreData, ErrorData } from '../types';

interface UseSystemStatsReturn {
  cpu: CPUUsageData | null;
  memory: MemoryUsageData | null;
  cores: CPUCoreData[];
  isConnected: boolean;
  error: ErrorData | null;
}

// Declare Tauri globals
declare global {
  interface Window {
    __TAURI__: any;
  }
}

const isTauri = () => typeof window !== 'undefined' && window.__TAURI__ !== undefined;

export const useSystemStats = (refreshInterval: number = 1000): UseSystemStatsReturn => {
  const [cpu, setCpu] = useState<CPUUsageData | null>(null);
  const [memory, setMemory] = useState<MemoryUsageData | null>(null);
  const [cores, setCores] = useState<CPUCoreData[]>([]);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState<ErrorData | null>(null);
  
  const intervalRef = useRef<number | null>(null);

  const fetchSystemStats = async () => {
    try {
      if (isTauri()) {
        // Use Tauri commands
        const stats: SystemStats = await invoke('get_system_stats');
        setCpu({ ...stats.cpu, timestamp: stats.timestamp });
        setMemory({ ...stats.memory, timestamp: stats.timestamp });
        setCores(stats.cpu.cores);
        setIsConnected(true);
        setError(null);
      } else {
        // Fallback to HTTP API for other backends
        const statsResponse = await fetch('http://localhost:3002/api/stats');

        if (statsResponse.ok) {
          const stats = await statsResponse.json();
          
          setCpu({ ...stats.cpu, timestamp: stats.timestamp });
          setMemory({ ...stats.memory, timestamp: stats.timestamp });
          setCores(stats.cpu.cores);
          setIsConnected(true);
          setError(null);
        } else {
          throw new Error('HTTP API request failed');
        }
      }
    } catch (err) {
      console.error('Failed to fetch system stats:', err);
      setIsConnected(false);
      setError({
        code: 'SYSTEM_ERROR',
        message: err instanceof Error ? err.message : 'Failed to fetch system stats',
        retry: true,
      });
    }
  };

  const startPolling = () => {
    // Fetch immediately
    fetchSystemStats();
    
    // Then poll at intervals
    intervalRef.current = setInterval(fetchSystemStats, refreshInterval) as unknown as number;
  };

  const stopPolling = () => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current as number);
      intervalRef.current = null;
    }
  };

  useEffect(() => {
    startPolling();
    
    return () => {
      stopPolling();
    };
  }, [refreshInterval]);

  const retryConnection = () => {
    setError(null);
    startPolling();
  };

  return {
    cpu,
    memory,
    cores,
    isConnected,
    error: error && error.retry ? { ...error, retry: retryConnection } : error,
  };
};