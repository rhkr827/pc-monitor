import React from 'react';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { MemoryStick } from 'lucide-react';
import { MemoryUsageData, ChartDataPoint } from '../types';
import { formatBytes, formatPercent, getPerformanceColor } from '../utils/formatters';

interface MemoryMonitorProps {
  data: ChartDataPoint[];
  currentData: MemoryUsageData;
  showBreakdown?: boolean;
}

export const MemoryMonitor: React.FC<MemoryMonitorProps> = ({
  data,
  currentData,
  showBreakdown = true,
}) => {
  const formatXAxis = (tickItem: number) => {
    const date = new Date(tickItem);
    return date.toLocaleTimeString('ko-KR', { 
      hour: '2-digit', 
      minute: '2-digit', 
      second: '2-digit' 
    });
  };

  const memoryColorClass = getPerformanceColor(currentData.usagePercent, 'memory');

  return (
    <div className="bg-bg-secondary rounded-lg p-6 border border-gray-700">
      <div className="flex items-center gap-2 mb-4">
        <MemoryStick className="w-5 h-5 text-cyan-400" />
        <h2 className="text-lg font-semibold">Memory Usage</h2>
      </div>

      <div className="grid grid-cols-2 lg:grid-cols-4 gap-4 mb-4">
        <div className="bg-bg-primary rounded p-4">
          <div className="text-sm text-text-secondary mb-1">Used</div>
          <div className={`text-xl font-bold ${memoryColorClass}`}>
            {formatBytes(currentData.used)}
          </div>
          <div className="text-sm text-text-secondary">
            {formatPercent(currentData.usagePercent)}
          </div>
        </div>
        
        <div className="bg-bg-primary rounded p-4">
          <div className="text-sm text-text-secondary mb-1">Total</div>
          <div className="text-xl font-bold text-blue-400">
            {formatBytes(currentData.total)}
          </div>
        </div>

        <div className="bg-bg-primary rounded p-4">
          <div className="text-sm text-text-secondary mb-1">Available</div>
          <div className="text-xl font-bold text-green-400">
            {formatBytes(currentData.available)}
          </div>
        </div>

        {showBreakdown && (
          <div className="bg-bg-primary rounded p-4">
            <div className="text-sm text-text-secondary mb-1">Cache</div>
            <div className="text-xl font-bold text-purple-400">
              {formatBytes(currentData.cache)}
            </div>
          </div>
        )}
      </div>

      {showBreakdown && (
        <div className="mb-4">
          <div className="flex justify-between text-sm text-text-secondary mb-2">
            <span>Memory Breakdown</span>
            <span>{formatBytes(currentData.total)}</span>
          </div>
          <div className="w-full bg-gray-700 rounded-full h-3 overflow-hidden">
            <div className="h-full flex">
              <div 
                className="bg-memory-safe h-full"
                style={{ width: `${(currentData.used / currentData.total) * 100}%` }}
                title={`Used: ${formatBytes(currentData.used)}`}
              />
              <div 
                className="bg-purple-500 h-full"
                style={{ width: `${(currentData.cache / currentData.total) * 100}%` }}
                title={`Cache: ${formatBytes(currentData.cache)}`}
              />
              <div 
                className="bg-gray-600 h-full"
                style={{ width: `${(currentData.buffers / currentData.total) * 100}%` }}
                title={`Buffers: ${formatBytes(currentData.buffers)}`}
              />
            </div>
          </div>
          <div className="flex justify-between text-xs text-text-secondary mt-1">
            <span>Used: {formatBytes(currentData.used)}</span>
            <span>Cache: {formatBytes(currentData.cache)}</span>
            <span>Buffers: {formatBytes(currentData.buffers)}</span>
          </div>
        </div>
      )}

      <div className="h-48">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={data}>
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
              formatter={(value: number) => [formatPercent(value), 'Memory Usage']}
            />
            <Area
              type="monotone"
              dataKey="value"
              stroke="#06b6d4"
              fill="url(#memoryGradient)"
              strokeWidth={2}
            />
            <defs>
              <linearGradient id="memoryGradient" x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor="#06b6d4" stopOpacity={0.8}/>
                <stop offset="95%" stopColor="#06b6d4" stopOpacity={0.1}/>
              </linearGradient>
            </defs>
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
};