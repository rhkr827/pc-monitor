use tauri::State;
use crate::monitor::SystemMonitor;
use crate::types::{CPUUsageData, MemoryUsageData, SystemStats};

#[tauri::command]
pub async fn get_cpu_stats(monitor: State<'_, SystemMonitor>) -> Result<CPUUsageData, String> {
    let mut rx = monitor.subscribe();
    match rx.recv().await {
        Ok(stats) => Ok(stats.cpu),
        Err(_) => Err("Failed to get CPU stats".to_string()),
    }
}

#[tauri::command]
pub async fn get_memory_stats(monitor: State<'_, SystemMonitor>) -> Result<MemoryUsageData, String> {
    let mut rx = monitor.subscribe();
    match rx.recv().await {
        Ok(stats) => Ok(stats.memory),
        Err(_) => Err("Failed to get memory stats".to_string()),
    }
}

#[tauri::command]
pub async fn get_system_stats(monitor: State<'_, SystemMonitor>) -> Result<SystemStats, String> {
    let mut rx = monitor.subscribe();
    match rx.recv().await {
        Ok(stats) => Ok(stats),
        Err(_) => Err("Failed to get system stats".to_string()),
    }
}