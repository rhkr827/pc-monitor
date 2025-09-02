use std::sync::Arc;
use std::time::Duration;
use sysinfo::{System, SystemExt, CpuExt};
use tokio::sync::broadcast;
use tokio::time::interval;

use crate::types::{SystemStats, CPUUsageData, MemoryUsageData, CPUCoreData};

#[derive(Clone)]
pub struct SystemMonitor {
    system: Arc<tokio::sync::Mutex<System>>,
    sender: broadcast::Sender<SystemStats>,
}

impl SystemMonitor {
    pub fn new() -> Self {
        let (sender, _) = broadcast::channel(100);
        Self {
            system: Arc::new(tokio::sync::Mutex::new(System::new_all())),
            sender,
        }
    }

    pub async fn start_monitoring(&self) {
        let system = Arc::clone(&self.system);
        let sender = self.sender.clone();
        
        tokio::spawn(async move {
            let mut interval = interval(Duration::from_secs(1));
            
            loop {
                interval.tick().await;
                
                let stats = {
                    let mut sys = system.lock().await;
                    sys.refresh_all();
                    
                    let cpu_usage = sys.global_cpu_info().cpu_usage();
                    let total_memory = sys.total_memory();
                    let used_memory = sys.used_memory();
                    let available_memory = sys.available_memory();
                    
                    let cores: Vec<CPUCoreData> = sys
                        .cpus()
                        .iter()
                        .enumerate()
                        .map(|(i, cpu)| CPUCoreData {
                            core_id: i,
                            usage: cpu.cpu_usage(),
                            frequency: cpu.frequency() as f32,
                        })
                        .collect();

                    let avg_frequency = cores.iter().map(|c| c.frequency).sum::<f32>() / cores.len() as f32;

                    SystemStats {
                        cpu: CPUUsageData {
                            overall: cpu_usage,
                            temperature: None, // TODO: Implement temperature reading
                            average_frequency: avg_frequency,
                        },
                        memory: MemoryUsageData {
                            total: total_memory,
                            used: used_memory,
                            available: available_memory,
                            cache: 0, // TODO: Implement cache reading
                            buffers: 0, // TODO: Implement buffers reading
                            usage_percent: (used_memory as f32 / total_memory as f32) * 100.0,
                        },
                        cores,
                        timestamp: std::time::SystemTime::now()
                            .duration_since(std::time::UNIX_EPOCH)
                            .unwrap()
                            .as_millis() as u64,
                    }
                };
                
                if sender.send(stats).is_err() {
                    println!("No receivers, continuing...");
                }
            }
        });
    }

    pub fn subscribe(&self) -> broadcast::Receiver<SystemStats> {
        self.sender.subscribe()
    }
}