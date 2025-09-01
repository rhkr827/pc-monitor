import { useState, useCallback, useRef, useEffect } from 'react';
import { ChartDataPoint } from '../types';

interface UseRealTimeChartReturn {
  chartData: ChartDataPoint[];
  addDataPoint: (point: ChartDataPoint) => void;
  clearData: () => void;
}

export const useRealTimeChart = (
  initialData: ChartDataPoint[] = [],
  maxDataPoints: number = 60
): UseRealTimeChartReturn => {
  const [chartData, setChartData] = useState<ChartDataPoint[]>([]);
  const dataBufferRef = useRef<ChartDataPoint[]>([]);

  const addDataPoint = useCallback((point: ChartDataPoint) => {
    dataBufferRef.current = [...dataBufferRef.current, point].slice(-maxDataPoints);
    setChartData([...dataBufferRef.current]);
  }, [maxDataPoints]);

  const clearData = useCallback(() => {
    dataBufferRef.current = [];
    setChartData([]);
  }, []);

  useEffect(() => {
    if (initialData.length > 0) {
      const latestPoint = initialData[initialData.length - 1];
      addDataPoint(latestPoint);
    }
  }, [initialData, addDataPoint]);

  return {
    chartData,
    addDataPoint,
    clearData,
  };
};