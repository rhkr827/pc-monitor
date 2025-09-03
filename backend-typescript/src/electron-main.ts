import { app, BrowserWindow } from 'electron';
import * as path from 'path';
import { WebServer } from './web-server';
import { SystemMonitor } from './system-monitor';

let mainWindow: BrowserWindow | null = null;
let webServer: WebServer | null = null;

function createWindow(): void {
  console.log('🖥️  Creating Electron window...');
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    },
    title: 'PC Monitor (TypeScript)',
    show: false
  });

  console.log('🖥️  Electron window created');

  // Load frontend (development vs production)
  if (process.env.NODE_ENV === 'development') {
    console.log('🔍 Loading dev frontend from: http://localhost:5173');
    mainWindow.loadURL('http://localhost:5173');
  } else {
    console.log('🔍 Loading from localhost backend...');
    setTimeout(() => {
      mainWindow?.loadURL('http://localhost:3002');
    }, 2000);
  }
  
  mainWindow.once('ready-to-show', () => {
    console.log('✅ UI ready - showing window');
    mainWindow?.show();
  });

  mainWindow.webContents.on('did-fail-load', (event, errorCode, errorDescription) => {
    console.error('❌ Failed to load frontend:', errorCode, errorDescription);
  });

  if (process.env.NODE_ENV === 'development') {
    mainWindow.webContents.openDevTools();
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

async function startBackend(): Promise<void> {
  try {
    console.log('🚀 Starting backend server...');
    const monitor = new SystemMonitor();
    const initialized = await monitor.initialize();
    
    if (!initialized) {
      console.error('❌ Failed to initialize system monitor');
      return;
    }
    
    console.log('✅ System monitor initialized');
    
    const PORT = 3002;
    webServer = new WebServer(monitor, PORT);
    const serverStarted = await webServer.start();
    
    if (!serverStarted) {
      console.error('❌ Failed to start web server');
      return;
    }
    
    console.log(`✅ Backend server running on http://localhost:${PORT}`);
  } catch (error) {
    console.error('❌ Backend startup error:', error);
  }
}

app.whenReady().then(async () => {
  await startBackend();
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (webServer) {
    webServer.stop();
  }
  
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('before-quit', () => {
  if (webServer) {
    webServer.stop();
  }
});