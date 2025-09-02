import asyncio
import json
import time
from typing import Dict, List, Any
from fastapi import WebSocket

from .cpu_monitor import CPUMonitor
from .memory_monitor import MemoryMonitor


class SystemMonitor:
    def __init__(self):
        self.connections: List[WebSocket] = []
        self.running = False
        self.cpu_monitor = CPUMonitor()
        self.memory_monitor = MemoryMonitor()

    def get_system_stats(self) -> Dict[str, Any]:
        timestamp = int(time.time() * 1000)

        return {
            "cpu": self.cpu_monitor.get_cpu_data(),
            "memory": self.memory_monitor.get_memory_data(),
            "cores": self.cpu_monitor.get_core_data(),
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

                disconnected = []
                for websocket in self.connections:
                    try:
                        await websocket.send_text(json.dumps(message))
                    except:
                        disconnected.append(websocket)

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