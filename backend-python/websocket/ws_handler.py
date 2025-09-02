import json
import time
from fastapi import WebSocket, WebSocketDisconnect
from monitors.system_monitor import SystemMonitor

monitor = SystemMonitor()


async def websocket_stats(websocket: WebSocket):
    await websocket.accept()
    await monitor.add_connection(websocket)

    try:
        while True:
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