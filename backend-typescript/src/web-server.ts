import express from 'express';
import cors from 'cors';
import { createServer } from 'http';
import { WebSocketServer, WebSocket } from 'ws';
import { SystemMonitor } from './system-monitor';
import { WebSocketMessage } from './types';
import * as path from 'path';

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
    
    // Serve frontend static files
    let frontendPath: string;
    if (process.env.NODE_ENV === 'development') {
      frontendPath = path.join(__dirname, '../../../frontend/dist');
    } else {
      // In packaged app, frontend is in app.asar at /frontend/dist
      frontendPath = path.join(__dirname, '../frontend/dist');
    }
    console.log('📁 Serving static files from:', frontendPath);
    this.app.use(express.static(frontendPath));
    
    // Fallback for SPA routing
    this.app.get('*', (req, res) => {
      if (!req.path.startsWith('/api') && !req.path.startsWith('/ws')) {
        res.sendFile(path.join(frontendPath, 'index.html'));
      }
    });
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

    // Cores data endpoint
    this.app.get('/api/cores', async (req, res) => {
      try {
        const stats = await this.monitor.getSystemStats();
        res.json(stats.cpu.cores);
      } catch (error) {
        res.status(500).json({ error: 'Failed to get cores data' });
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