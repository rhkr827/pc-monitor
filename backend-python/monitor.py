import asyncio
import json
import time
import psutil
import websockets
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
from typing import Dict, List, Any

app = FastAPI(title="PC Monitor API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


class SystemMonitor:
    def __init__(self):
        self.connections: List[WebSocket] = []
        self.running = False

    def get_cpu_data(self) -> Dict[str, Any]:
        cpu_percent = psutil.cpu_percent(interval=0.1)
        cpu_freq = psutil.cpu_freq()

        return {
            "overall": cpu_percent,
            "temperature": None,  # Windows doesn't easily expose CPU temp via psutil
            "averageFrequency": cpu_freq.current if cpu_freq else 0.0,
        }

    def get_memory_data(self) -> Dict[str, Any]:
        memory = psutil.virtual_memory()

        return {
            "total": memory.total,
            "used": memory.used,
            "available": memory.available,
            "cache": getattr(memory, "cached", 0),
            "buffers": getattr(memory, "buffers", 0),
            "usagePercent": memory.percent,
        }

    def get_core_data(self) -> List[Dict[str, Any]]:
        cpu_percents = psutil.cpu_percent(percpu=True, interval=0.1)
        cpu_freq = psutil.cpu_freq(percpu=True)

        cores = []
        for i, usage in enumerate(cpu_percents):
            freq = cpu_freq[i].current if cpu_freq and i < len(cpu_freq) else 0.0
            cores.append({"coreId": i, "usage": usage, "frequency": freq})

        return cores

    def get_system_stats(self) -> Dict[str, Any]:
        timestamp = int(time.time() * 1000)

        return {
            "cpu": self.get_cpu_data(),
            "memory": self.get_memory_data(),
            "cores": self.get_core_data(),
            "timestamp": timestamp,
        }

    async def broadcast_stats(self):
        while self.running:
            try:
                stats = self.get_system_stats()
                message = {
                    "type": "stats",
                    "timestamp": stats["timestamp"],
                    "data": stats,
                }

                # Broadcast to all connected clients
                disconnected = []
                for websocket in self.connections:
                    try:
                        await websocket.send_text(json.dumps(message))
                    except:
                        disconnected.append(websocket)

                # Remove disconnected clients
                for ws in disconnected:
                    self.connections.remove(ws)

                await asyncio.sleep(1.0)
            except Exception as e:
                print(f"Error in broadcast_stats: {e}")
                await asyncio.sleep(1.0)

    async def add_connection(self, websocket: WebSocket):
        self.connections.append(websocket)
        if not self.running:
            self.running = True
            asyncio.create_task(self.broadcast_stats())

    async def remove_connection(self, websocket: WebSocket):
        if websocket in self.connections:
            self.connections.remove(websocket)

        if len(self.connections) == 0:
            self.running = False


monitor = SystemMonitor()


@app.get("/api/cpu")
async def get_cpu():
    return monitor.get_cpu_data()


@app.get("/api/memory")
async def get_memory():
    return monitor.get_memory_data()


@app.get("/api/cores")
async def get_cores():
    return monitor.get_core_data()


@app.get("/api/stats")
async def get_stats():
    return monitor.get_system_stats()


@app.websocket("/ws/stats")
async def websocket_stats(websocket: WebSocket):
    await websocket.accept()
    await monitor.add_connection(websocket)

    try:
        while True:
            # Keep connection alive
            data = await websocket.receive_text()
            if data == "ping":
                await websocket.send_text(
                    json.dumps(
                        {
                            "type": "heartbeat",
                            "timestamp": int(time.time() * 1000),
                            "data": None,
                        }
                    )
                )
    except WebSocketDisconnect:
        await monitor.remove_connection(websocket)
    except Exception as e:
        print(f"WebSocket error: {e}")
        await monitor.remove_connection(websocket)


if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=8080, log_level="info")
