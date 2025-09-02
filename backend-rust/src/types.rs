use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SystemStats {
    pub cpu: CPUUsageData,
    pub memory: MemoryUsageData,
    pub cores: Vec<CPUCoreData>,
    pub timestamp: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CPUUsageData {
    pub overall: f32,
    pub temperature: Option<f32>,
    #[serde(rename = "averageFrequency")]
    pub average_frequency: f32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MemoryUsageData {
    pub total: u64,
    pub used: u64,
    pub available: u64,
    pub cache: u64,
    pub buffers: u64,
    #[serde(rename = "usagePercent")]
    pub usage_percent: f32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CPUCoreData {
    #[serde(rename = "coreId")]
    pub core_id: usize,
    pub usage: f32,
    pub frequency: f32,
}