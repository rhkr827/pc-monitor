import express from 'express';
import cors from 'cors';
import { createServer } from 'http';
import { WebSocketServer, WebSocket } from 'ws';
import { SystemMonitor } from './system-monitor';
import { WebSocketMessage } from './types';

export class WebServer {
  private app: express.Application;
  private server: any;
  private wss: WebSocketServer | null = null;
  private monitor: SystemMonitor;
  private port: number;
  private clients: Set<WebSocket> = new Set();

  constructor(monitor: SystemMonitor, port: number = 3002) {
    this.monitor = monitor;
    this.port = port;
    this.app = express();
    this.setupMiddleware();
    this.setupRoutes();
  }

  private setupMiddleware(): void {
    this.app.use(cors());
    this.app.use(express.json());
  }

  private setupRoutes(): void {
    // Health check
    this.app.get('/health', (req, res) => {
      res.json({ status: 'ok', timestamp: Date.now() });
    });

    // CPU data endpoint
    this.app.get('/api/cpu', async (req, res) => {
      try {
        const cpuData = await this.monitor.getCpuData();
        res.json(cpuData);
      } catch (error) {
        res.status(500).json({ error: 'Failed to get CPU data' });
      }
    });

    // Memory data endpoint
    this.app.get('/api/memory', async (req, res) => {
      try {
        const memoryData = await this.monitor.getMemoryData();
        res.json(memoryData);
      } catch (error) {
        res.status(500).json({ error: 'Failed to get memory data' });
      }
    });

    // Complete system stats endpoint
    this.app.get('/api/stats', async (req, res) => {
      try {
        const stats = await this.monitor.getSystemStats();
        res.json(stats);
      } catch (error) {
        res.status(500).json({ error: 'Failed to get system stats' });
      }
    });
  }

  async start(): Promise<boolean> {
    try {
      this.server = createServer(this.app);
      
      // Setup WebSocket server
      this.wss = new WebSocketServer({ server: this.server });
      this.setupWebSocket();

      // Start listening
      await new Promise<void>((resolve, reject) => {
        this.server.listen(this.port, (error: any) => {
          if (error) {
            reject(error);
          } else {
            resolve();
          }
        });
      });

      // Start monitoring
      this.monitor.startMonitoring(1000);
      this.monitor.addListener(this.broadcastStats.bind(this));

      return true;
    } catch (error) {
      console.error('Failed to start web server:', error);
      return false;
    }
  }

  private setupWebSocket(): void {
    if (!this.wss) return;

    this.wss.on('connection', (ws: WebSocket) => {
      this.clients.add(ws);
      console.log(`Client connected. Total clients: ${this.clients.size}`);

      ws.on('message', (data: string) => {
        try {
          if (data.toString() === 'ping') {
            const response: WebSocketMessage = {
              type: 'heartbeat',
              timestamp: Date.now(),
              data: null
            };
            ws.send(JSON.stringify(response));
          }
        } catch (error) {
          console.error('WebSocket message error:', error);
        }
      });

      ws.on('close', () => {
        this.clients.delete(ws);
        console.log(`Client disconnected. Total clients: ${this.clients.size}`);
      });

      ws.on('error', (error) => {
        console.error('WebSocket error:', error);
        this.clients.delete(ws);
      });
    });
  }

  private broadcastStats(stats: any): void {
    if (this.clients.size === 0) return;

    const message: WebSocketMessage = {
      type: 'stats',
      timestamp: stats.timestamp,
      data: stats
    };

    const messageStr = JSON.stringify(message);
    const disconnectedClients: WebSocket[] = [];

    this.clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {
        try {
          client.send(messageStr);
        } catch (error) {
          console.error('Failed to send to client:', error);
          disconnectedClients.push(client);
        }
      } else {
        disconnectedClients.push(client);
      }
    });

    // Clean up disconnected clients
    disconnectedClients.forEach(client => {
      this.clients.delete(client);
    });
  }

  stop(): void {
    this.monitor.stopMonitoring();
    
    if (this.wss) {
      this.wss.close();
    }
    
    if (this.server) {
      this.server.close();
    }
  }

  isRunning(): boolean {
    return this.server?.listening || false;
  }
}