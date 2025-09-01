# PC Monitor - Setup & Development Guide

## 🚀 Quick Start

### Prerequisites
- **Node.js** 18+ 
- **Python** 3.8+ (for Python backend)
- **Rust** 1.70+ (for Rust backend)

### Installation

```bash
# Install all dependencies
npm run install:all

# For Rust backend only
npm run setup:rust
```

## 🏃‍♂️ Development

### Frontend Only (React Development)
```bash
npm run dev:frontend
# Opens browser at http://localhost:3000
```

### Rust + Tauri Backend
```bash
npm run dev:rust
# Builds Rust backend + opens desktop app
```

### Python + Electron Backend  
```bash
npm run dev:python
# Starts Python FastAPI server + Electron app
```

## 🏗️ Building

### Production Builds
```bash
# Frontend only
npm run build:frontend

# Rust desktop app
npm run build:rust

# Python desktop app
npm run build:python
```

## 📁 Project Structure

```
pc-monitor/
├── frontend/                 # React TypeScript UI (shared)
│   ├── src/
│   │   ├── components/       # Dashboard, CPU, Memory, CoreGrid
│   │   ├── hooks/           # useSystemStats, useRealTimeChart
│   │   ├── types/           # TypeScript interfaces
│   │   └── utils/           # formatters, helpers
├── backend-rust/             # Tauri + Rust backend
│   ├── src/main.rs          # Rust system monitoring
│   └── Cargo.toml
├── backend-python/           # Electron + Python backend
│   ├── monitor.py           # Python FastAPI server
│   ├── src/main.js          # Electron main process
│   └── requirements.txt
└── package.json             # Root scripts
```

## 🔧 API Endpoints

All backends expose the same REST API:

- `GET /api/cpu` → CPU usage data
- `GET /api/memory` → Memory usage data  
- `GET /api/stats` → Complete system stats
- `WS /ws/stats` → Real-time data stream

## 🎯 Performance Comparison

| Backend | Memory | Binary Size | Cold Start | Hot Performance |
|---------|--------|-------------|------------|----------------|
| Rust    | ~50MB  | ~15MB       | 2-3s       | ⭐⭐⭐⭐⭐        |
| Python  | ~150MB | ~100MB      | 3-5s       | ⭐⭐⭐⭐          |

## 🐛 Troubleshooting

### Python Backend Issues
```bash
# Install Python dependencies
pip install -r backend-python/requirements.txt

# Check Python version
python --version  # Should be 3.8+
```

### Rust Backend Issues
```bash
# Install Rust if not available
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Update Rust
rustup update
```

### Port Conflicts
- Frontend: Port 3000 (Vite dev server)
- Backend API: Port 8080 (FastAPI/Tauri)
- WebSocket: Port 8080/ws/stats

Change ports in respective config files if needed.