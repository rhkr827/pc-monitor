import { useState, useEffect, useRef } from 'react';
import { SystemStats, CPUUsageData, MemoryUsageData, CPUCoreData, ErrorData } from '../types';

interface UseSystemStatsReturn {
  cpu: CPUUsageData | null;
  memory: MemoryUsageData | null;
  cores: CPUCoreData[];
  isConnected: boolean;
  error: ErrorData | null;
}

export const useSystemStats = (refreshInterval: number = 1000): UseSystemStatsReturn => {
  const [cpu, setCpu] = useState<CPUUsageData | null>(null);
  const [memory, setMemory] = useState<MemoryUsageData | null>(null);
  const [cores, setCores] = useState<CPUCoreData[]>([]);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState<ErrorData | null>(null);
  
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<NodeJS.Timeout | null>(null);
  const reconnectAttempts = useRef(0);
  const maxReconnectAttempts = 5;

  const connectWebSocket = () => {
    try {
      const ws = new WebSocket('ws://localhost:8080/ws/stats');
      wsRef.current = ws;

      ws.onopen = () => {
        console.log('WebSocket connected');
        setIsConnected(true);
        setError(null);
        reconnectAttempts.current = 0;
      };

      ws.onmessage = (event) => {
        try {
          const message = JSON.parse(event.data);
          
          if (message.type === 'stats' && message.data) {
            const stats: SystemStats = message.data;
            setCpu({ ...stats.cpu, timestamp: stats.timestamp });
            setMemory({ ...stats.memory, timestamp: stats.timestamp });
            setCores(stats.cores);
          } else if (message.type === 'error') {
            setError(message.data);
          }
        } catch (err) {
          console.error('Failed to parse WebSocket message:', err);
        }
      };

      ws.onclose = () => {
        console.log('WebSocket disconnected');
        setIsConnected(false);
        
        if (reconnectAttempts.current < maxReconnectAttempts) {
          const delay = Math.min(1000 * Math.pow(2, reconnectAttempts.current), 30000);
          reconnectTimeoutRef.current = setTimeout(() => {
            reconnectAttempts.current++;
            connectWebSocket();
          }, delay);
        } else {
          setError({
            code: 'CONNECTION_LOST',
            message: 'Failed to reconnect after multiple attempts',
            retry: true,
          });
        }
      };

      ws.onerror = (err) => {
        console.error('WebSocket error:', err);
        setError({
          code: 'SYSTEM_ERROR',
          message: 'WebSocket connection error',
          retry: true,
        });
      };
    } catch (err) {
      console.error('Failed to create WebSocket connection:', err);
      setError({
        code: 'CONNECTION_LOST',
        message: 'Unable to establish connection',
        retry: true,
      });
    }
  };

  const fetchInitialData = async () => {
    try {
      const [cpuResponse, memoryResponse] = await Promise.all([
        fetch('http://localhost:8080/api/cpu'),
        fetch('http://localhost:8080/api/memory'),
      ]);

      if (cpuResponse.ok && memoryResponse.ok) {
        const cpuData = await cpuResponse.json();
        const memoryData = await memoryResponse.json();
        
        setCpu(cpuData);
        setMemory(memoryData);
      }
    } catch (err) {
      console.error('Failed to fetch initial data:', err);
    }
  };

  useEffect(() => {
    fetchInitialData();
    connectWebSocket();

    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
    };
  }, []);

  const retryConnection = () => {
    reconnectAttempts.current = 0;
    setError(null);
    connectWebSocket();
  };

  return {
    cpu,
    memory,
    cores,
    isConnected,
    error: error && error.retry ? { ...error, retry: retryConnection } : error,
  };
};