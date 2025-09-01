export const formatBytes = (bytes: number): string => {
  if (bytes === 0) return '0 B';
  
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  
  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(1))} ${sizes[i]}`;
};

export const formatPercent = (value: number): string => {
  return `${Math.round(value)}%`;
};

export const formatFrequency = (mhz: number): string => {
  if (mhz >= 1000) {
    return `${(mhz / 1000).toFixed(1)} GHz`;
  }
  return `${Math.round(mhz)} MHz`;
};

export const formatTemperature = (celsius: number): string => {
  return `${Math.round(celsius)}°C`;
};

export const getPerformanceColor = (percentage: number, type: 'cpu' | 'memory'): string => {
  if (type === 'cpu') {
    if (percentage <= 50) return 'text-cpu-low';
    if (percentage <= 80) return 'text-cpu-medium';
    return 'text-cpu-high';
  } else {
    if (percentage <= 60) return 'text-memory-safe';
    if (percentage <= 85) return 'text-memory-warning';
    return 'text-memory-critical';
  }
};

export const getPerformanceBgColor = (percentage: number, type: 'cpu' | 'memory'): string => {
  if (type === 'cpu') {
    if (percentage <= 50) return 'bg-cpu-low';
    if (percentage <= 80) return 'bg-cpu-medium';
    return 'bg-cpu-high';
  } else {
    if (percentage <= 60) return 'bg-memory-safe';
    if (percentage <= 85) return 'bg-memory-warning';
    return 'bg-memory-critical';
  }
};