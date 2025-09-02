from fastapi import APIRouter
from monitors.system_monitor import SystemMonitor

router = APIRouter(prefix="/api")
monitor = SystemMonitor()


@router.get("/cpu")
async def get_cpu():
    return monitor.cpu_monitor.get_cpu_data()


@router.get("/memory")
async def get_memory():
    return monitor.memory_monitor.get_memory_data()


@router.get("/cores")
async def get_cores():
    return monitor.cpu_monitor.get_core_data()


@router.get("/stats")
async def get_stats():
    return monitor.get_system_stats()