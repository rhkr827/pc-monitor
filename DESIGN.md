# PC Monitor - System Architecture Design

## 📋 Project Overview
**Target**: Real-time PC performance monitoring desktop application for Windows
**Features**: Multi-language backend support, real-time data visualization, modular architecture

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     PC Monitor Application                      │
├─────────────────────────────────────────────────────────────────┤
│  Frontend Layer (Shared)                                       │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │ React + TypeScript + Vite + Tailwind + Recharts            │ │
│  │ ├── components/   (Dashboard, CPU, Memory, CoreGrid)       │ │
│  │ ├── hooks/        (useSystemStats, useRealTimeData)        │ │
│  │ ├── types/        (SystemStats, ChartData interfaces)      │ │
│  │ └── utils/        (formatters, chart helpers)              │ │
│  └─────────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  Communication Layer                                           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │ REST API + WebSocket Interface                              │ │
│  │ GET /api/cpu     → CPU usage, core data, temperature       │ │
│  │ GET /api/memory  → Total, used, cache, buffers             │ │
│  │ WS  /ws/stats    → Real-time data stream (1s interval)     │ │
│  └─────────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  Backend Implementations (Multi-Language)                      │
│  ┌───────────┬───────────┬─────────┬─────────┬─────────────────┐ │
│  │   Rust    │TypeScript │ Python  │   C#    │      C++        │ │
│  │  (Tauri)  │(Electron) │(Electron│(Electron│   (Electron)    │ │
│  │           │           │    )    │    )    │                 │ │
│  │ sysinfo   │systeminf. │ psutil  │PerformC │  Windows API    │ │
│  │ crate     │ library   │ library │ Counter │  direct calls   │ │
│  │ ⭐⭐⭐⭐⭐  │ ⭐⭐⭐⭐⭐   │ ⭐⭐⭐⭐⭐ │ ⭐⭐⭐⭐   │ ⭐⭐⭐           │ │
│  └───────────┴───────────┴─────────┴─────────┴─────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 📊 Data Model Design

### Core Interfaces
```typescript
interface SystemStats {
  cpu: CPUUsageData;
  memory: MemoryUsageData;
  cores: CPUCoreData[];
  timestamp: number;
}

interface CPUUsageData {
  overall: number;        // 0-100%
  temperature?: number;   // Celsius
  averageFrequency: number; // MHz
}

interface MemoryUsageData {
  total: number;          // bytes
  used: number;           // bytes
  available: number;      // bytes
  cache: number;          // bytes
  buffers: number;        // bytes
  usagePercent: number;   // calculated 0-100%
}

interface CPUCoreData {
  coreId: number;
  usage: number;          // 0-100%
  frequency: number;      // MHz
}
```

### WebSocket Protocol
```typescript
interface WebSocketMessage {
  type: 'stats' | 'error' | 'heartbeat';
  timestamp: number;
  data: SystemStats | ErrorData | null;
}

interface ErrorData {
  code: 'PERMISSION_DENIED' | 'SYSTEM_ERROR' | 'CONNECTION_LOST';
  message: string;
  retry: boolean;
}
```

## 🔄 Real-Time Data Flow

### Frontend Data Management
```
┌─────────────────────────────────────────────────────────────────┐
│ React Component Hierarchy                                       │
│                                                                 │
│ App                                                             │
│ └── Dashboard                                                   │
│     ├── CPUMonitor ←── useSystemStats() ←── WebSocket          │
│     ├── MemoryMonitor ←── useRealTimeChart() ←── Data Buffer    │
│     └── CPUCoreGrid ←── useCoreData() ←── State Management      │
│                                                                 │
│ Data Flow: WebSocket → Custom Hooks → Component State → UI     │
└─────────────────────────────────────────────────────────────────┘
```

### Backend Polling Strategy
```
System Monitor Service
├── Data Collectors (1초 간격)
│   ├── CPU Collector → sysinfo/psutil/PerformanceCounter
│   ├── Memory Collector → 메모리 통계 수집
│   └── Core Collector → 개별 코어 데이터
├── Data Processor
│   ├── Validation → 데이터 무결성 검증
│   ├── Formatting → 공통 인터페이스 변환
│   └── Caching → 최근 60개 데이터 포인트 보관
└── Broadcasting
    ├── WebSocket → 실시간 스트림 (연결된 클라이언트)
    └── REST API → 요청 시 현재 상태 제공
```

## 🎨 UI/UX Design Specifications

### Layout Structure
```
┌─────────────────────────────────────────────────────────────────┐
│ Header: PC Performance Monitor                          [⚙️📱] │
├─────────────────────┬───────────────────────────────────────────┤
│   CPU Overall       │          Memory Usage                     │
│ ┌─────────────────┐ │ ┌───────────────────────────────────────┐ │
│ │ 📈 Line Chart   │ │ │ 📊 Area Chart                         │ │
│ │ Current: 45%    │ │ │ Used: 8.2GB / 16GB (51%)              │ │
│ │ Temp: 67°C      │ │ │ Cache: 2.1GB | Available: 7.8GB      │ │
│ └─────────────────┘ │ └───────────────────────────────────────┘ │
├─────────────────────┴───────────────────────────────────────────┤
│                   CPU Cores (Individual)                       │
│ ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐ │
│ │ C0 │ C1 │ C2 │ C3 │ C4 │ C5 │ C6 │ C7 │ C8 │ C9 │C10 │C11 │ │
│ │75% │45% │89% │23% │67% │34% │56% │78% │12% │91% │43% │65% │ │
│ │▓▓▓│▓▓ │▓▓▓│▓  │▓▓▓│▓▓ │▓▓▓│▓▓▓│▓  │▓▓▓│▓▓ │▓▓▓│ │
│ │2.8G│3.1G│2.9G│3.2G│2.7G│3.0G│2.8G│2.9G│3.3G│2.6G│3.1G│2.8G│ │
│ └────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Color Scheme (Dark Theme)
```css
:root {
  --bg-primary: #1a1a1a;
  --bg-secondary: #2d2d2d;
  --text-primary: #ffffff;
  --text-secondary: #b3b3b3;
  
  /* Performance-based colors */
  --cpu-low: #22c55e;      /* 0-50% */
  --cpu-medium: #eab308;   /* 51-80% */
  --cpu-high: #ef4444;     /* 81-100% */
  
  --memory-safe: #06b6d4;
  --memory-warning: #f59e0b;
  --memory-critical: #dc2626;
}
```

### Responsive Breakpoints
```
Desktop: 1200px+ → Full 2x2 grid layout
Tablet:  768px+  → Stacked layout with horizontal cores
Mobile:  <768px  → Single column, compact cores
```

## 🔌 Backend Implementation Strategy

### Framework Decision Matrix
| Framework | Memory | Binary Size | Development Speed | Platform Support |
|-----------|--------|-------------|-------------------|------------------|
| Tauri     | ~50MB  | ~15MB       | Medium           | Rust only        |
| Electron  | ~150MB | ~100MB      | Fast             | All languages    |
