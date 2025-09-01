const { app, BrowserWindow } = require('electron');
const { spawn } = require('child_process');
const path = require('path');

let mainWindow;
let pythonProcess;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    minWidth: 800,
    minHeight: 600,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      webSecurity: false
    },
    autoHideMenuBar: true,
    title: 'PC Performance Monitor (Python)'
  });

  // Start Python backend
  startPythonBackend();

  // Wait a moment for Python server to start, then load the frontend
  setTimeout(() => {
    mainWindow.loadURL('http://localhost:3000');
  }, 2000);

  mainWindow.on('closed', () => {
    mainWindow = null;
    if (pythonProcess) {
      pythonProcess.kill();
    }
  });
}

function startPythonBackend() {
  const pythonScript = path.join(__dirname, '..', 'monitor.py');
  
  pythonProcess = spawn('python', [pythonScript], {
    stdio: ['pipe', 'pipe', 'pipe']
  });

  pythonProcess.stdout.on('data', (data) => {
    console.log(`Python stdout: ${data}`);
  });

  pythonProcess.stderr.on('data', (data) => {
    console.error(`Python stderr: ${data}`);
  });

  pythonProcess.on('close', (code) => {
    console.log(`Python process exited with code ${code}`);
  });

  pythonProcess.on('error', (err) => {
    console.error('Failed to start Python process:', err);
  });
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (pythonProcess) {
    pythonProcess.kill();
  }
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});