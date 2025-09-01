export interface CpuData {
  overall: number;
  temperature?: number;
  averageFrequency: number;
  cores: CoreData[];
}

export interface CoreData {
  coreId: number;
  usage: number;
  frequency: number;
}

export interface MemoryData {
  total: number;
  used: number;
  available: number;
  cache: number;
  buffers: number;
  usagePercent: number;
}

export interface SystemStats {
  cpu: CpuData;
  memory: MemoryData;
  timestamp: number;
}

export interface WebSocketMessage {
  type: string;
  timestamp: number;
  data: any;
}