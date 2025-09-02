import psutil
from typing import Dict, List, Any


class CPUMonitor:
    def get_cpu_data(self) -> Dict[str, Any]:
        cpu_percent = psutil.cpu_percent(interval=0.1)
        cpu_freq = psutil.cpu_freq()

        return {
            "overall": cpu_percent,
            "temperature": None,
            "averageFrequency": cpu_freq.current if cpu_freq else 0.0,
        }

    def get_core_data(self) -> List[Dict[str, Any]]:
        cpu_percents = psutil.cpu_percent(percpu=True, interval=0.1)
        cpu_freq = psutil.cpu_freq(percpu=True)

        cores = []
        for i, usage in enumerate(cpu_percents):
            freq = cpu_freq[i].current if cpu_freq and i < len(cpu_freq) else 0.0
            cores.append({"coreId": i, "usage": usage, "frequency": freq})

        return cores