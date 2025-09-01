export interface SystemStats {
  cpu: CPUUsageData;
  memory: MemoryUsageData;
  cores: CPUCoreData[];
  timestamp: number;
}

export interface CPUUsageData {
  overall: number;        // 0-100%
  temperature?: number;   // Celsius
  averageFrequency: number; // MHz
  timestamp?: number;     // Unix timestamp
}

export interface MemoryUsageData {
  total: number;          // bytes
  used: number;           // bytes
  available: number;      // bytes
  cache: number;          // bytes
  buffers: number;        // bytes
  usagePercent: number;   // calculated 0-100%
  timestamp?: number;     // Unix timestamp
}

export interface CPUCoreData {
  coreId: number;
  usage: number;          // 0-100%
  frequency: number;      // MHz
}

export interface WebSocketMessage {
  type: 'stats' | 'error' | 'heartbeat';
  timestamp: number;
  data: SystemStats | ErrorData | null;
}

export interface ErrorData {
  code: 'PERMISSION_DENIED' | 'SYSTEM_ERROR' | 'CONNECTION_LOST';
  message: string;
  retry: boolean | (() => void);
}

export interface ChartDataPoint {
  timestamp: number;
  value: number;
}

export type TimeRange = '1m' | '5m' | '1h';
export type ThemeMode = 'light' | 'dark';