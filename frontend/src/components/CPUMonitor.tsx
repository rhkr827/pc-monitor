import React from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { Cpu, Thermometer } from 'lucide-react';
import { CPUUsageData, ChartDataPoint, TimeRange } from '../types';
import { formatPercent, formatTemperature, getPerformanceColor } from '../utils/formatters';

interface CPUMonitorProps {
  data: ChartDataPoint[];
  currentData: CPUUsageData;
  timeRange: TimeRange;
  onTimeRangeChange: (range: TimeRange) => void;
}

export const CPUMonitor: React.FC<CPUMonitorProps> = ({
  data,
  currentData,
  timeRange,
  onTimeRangeChange,
}) => {
  const formatXAxis = (tickItem: number) => {
    const date = new Date(tickItem);
    return date.toLocaleTimeString('ko-KR', { 
      hour: '2-digit', 
      minute: '2-digit', 
      second: '2-digit' 
    });
  };

  const cpuColorClass = getPerformanceColor(currentData.overall, 'cpu');

  return (
    <div className="bg-bg-secondary rounded-lg p-6 border border-gray-700">
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center gap-2">
          <Cpu className="w-5 h-5 text-blue-400" />
          <h2 className="text-lg font-semibold">CPU Usage</h2>
        </div>
        <div className="flex gap-2">
          {(['1m', '5m', '1h'] as TimeRange[]).map((range) => (
            <button
              key={range}
              onClick={() => onTimeRangeChange(range)}
              className={`px-3 py-1 rounded text-sm ${
                timeRange === range
                  ? 'bg-blue-600 text-white'
                  : 'bg-gray-600 text-gray-300 hover:bg-gray-500'
              }`}
            >
              {range}
            </button>
          ))}
        </div>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-4 mb-4">
        <div className="bg-bg-primary rounded p-4">
          <div className="text-sm text-text-secondary mb-1">Current Usage</div>
          <div className={`text-2xl font-bold ${cpuColorClass}`}>
            {formatPercent(currentData.overall)}
          </div>
        </div>
        
        <div className="bg-bg-primary rounded p-4">
          <div className="text-sm text-text-secondary mb-1">Average Frequency</div>
          <div className="text-2xl font-bold text-blue-400">
            {(currentData.averageFrequency / 1000).toFixed(1)} GHz
          </div>
        </div>

        {currentData.temperature && (
          <div className="bg-bg-primary rounded p-4">
            <div className="flex items-center gap-1 text-sm text-text-secondary mb-1">
              <Thermometer className="w-4 h-4" />
              Temperature
            </div>
            <div className="text-2xl font-bold text-orange-400">
              {formatTemperature(currentData.temperature)}
            </div>
          </div>
        )}
      </div>

      <div className="h-64">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="#404040" />
            <XAxis 
              dataKey="timestamp"
              tickFormatter={formatXAxis}
              stroke="#b3b3b3"
              fontSize={12}
            />
            <YAxis 
              domain={[0, 100]}
              stroke="#b3b3b3"
              fontSize={12}
              tickFormatter={(value) => `${value}%`}
            />
            <Tooltip
              contentStyle={{
                backgroundColor: '#2d2d2d',
                border: '1px solid #404040',
                borderRadius: '8px',
                color: '#ffffff'
              }}
              labelFormatter={(label) => `Time: ${formatXAxis(label as number)}`}
              formatter={(value: number) => [formatPercent(value), 'CPU Usage']}
            />
            <Line
              type="monotone"
              dataKey="value"
              stroke="#3b82f6"
              strokeWidth={2}
              dot={false}
              activeDot={{ r: 4, fill: '#3b82f6' }}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
};