#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod types;
mod monitor;
mod handlers;

use tauri::Manager;
use monitor::SystemMonitor;
use handlers::{get_cpu_stats, get_memory_stats, get_system_stats};

fn main() {
    let monitor = SystemMonitor::new();
    
    tauri::Builder::default()
        .manage(monitor.clone())
        .setup(|app| {
            let monitor = app.state::<SystemMonitor>();
            let monitor_clone = monitor.inner().clone();
            tauri::async_runtime::spawn(async move {
                monitor_clone.start_monitoring().await;
            });
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            get_cpu_stats,
            get_memory_stats,
            get_system_stats
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}