import { app, BrowserWindow } from 'electron';
import * as path from 'path';
import { spawn } from 'child_process';

let mainWindow: BrowserWindow | null = null;
let backendProcess: any = null;

function createWindow(): void {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    },
    title: 'PC Monitor (TypeScript)',
    icon: path.join(__dirname, '../assets/icon.png')
  });

  // Load frontend (assuming it's running on port 5173 for Vite dev server)
  mainWindow.loadURL('http://localhost:5173');

  if (process.env.NODE_ENV === 'development') {
    mainWindow.webContents.openDevTools();
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

function startBackend(): void {
  // Start the TypeScript backend server
  backendProcess = spawn('node', [path.join(__dirname, 'main.js')], {
    stdio: 'inherit'
  });

  backendProcess.on('error', (error: Error) => {
    console.error('Backend process error:', error);
  });

  backendProcess.on('exit', (code: number | null) => {
    console.log(`Backend process exited with code ${code}`);
  });
}

app.whenReady().then(() => {
  startBackend();
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (backendProcess) {
    backendProcess.kill();
  }
  
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('before-quit', () => {
  if (backendProcess) {
    backendProcess.kill();
  }
});