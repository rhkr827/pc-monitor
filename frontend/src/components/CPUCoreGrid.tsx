import React from 'react';
import { Cpu } from 'lucide-react';
import { CPUCoreData } from '../types';
import { formatPercent, formatFrequency, getPerformanceBgColor } from '../utils/formatters';

interface CPUCoreGridProps {
  cores: CPUCoreData[];
  layout?: 'grid' | 'horizontal';
  showFrequency?: boolean;
}

export const CPUCoreGrid: React.FC<CPUCoreGridProps> = ({
  cores,
  layout = 'grid',
  showFrequency = true,
}) => {
  const gridCols = layout === 'grid' 
    ? cores.length <= 8 
      ? 'grid-cols-4 lg:grid-cols-8' 
      : 'grid-cols-6 lg:grid-cols-12'
    : 'grid-cols-4 lg:grid-cols-8';

  return (
    <div className="bg-bg-secondary rounded-lg p-6 border border-gray-700">
      <div className="flex items-center gap-2 mb-4">
        <Cpu className="w-5 h-5 text-green-400" />
        <h2 className="text-lg font-semibold">CPU Cores</h2>
        <span className="text-sm text-text-secondary">({cores.length} cores)</span>
      </div>

      <div className={`grid ${gridCols} gap-3`}>
        {cores.map((core) => {
          const bgColorClass = getPerformanceBgColor(core.usage, 'cpu');
          
          return (
            <div key={core.coreId} className="bg-bg-primary rounded-lg p-3 text-center">
              <div className="text-xs text-text-secondary mb-1">
                C{core.coreId}
              </div>
              
              <div className="relative mb-2">
                <div className="w-full bg-gray-700 rounded-full h-2 overflow-hidden">
                  <div 
                    className={`h-full ${bgColorClass} transition-all duration-300`}
                    style={{ width: `${core.usage}%` }}
                  />
                </div>
                <div className="text-sm font-bold mt-1">
                  {formatPercent(core.usage)}
                </div>
              </div>

              {showFrequency && (
                <div className="text-xs text-text-secondary">
                  {formatFrequency(core.frequency)}
                </div>
              )}
            </div>
          );
        })}
      </div>

      <div className="mt-4 flex justify-between items-center text-sm text-text-secondary">
        <div className="flex gap-4">
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 bg-cpu-low rounded-full"></div>
            <span>Low (0-50%)</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 bg-cpu-medium rounded-full"></div>
            <span>Medium (51-80%)</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-3 h-3 bg-cpu-high rounded-full"></div>
            <span>High (81-100%)</span>
          </div>
        </div>
        <div>
          Avg: {formatPercent(cores.reduce((sum, core) => sum + core.usage, 0) / cores.length)}
        </div>
      </div>
    </div>
  );
};