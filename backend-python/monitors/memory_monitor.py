import psutil
from typing import Dict, Any


class MemoryMonitor:
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