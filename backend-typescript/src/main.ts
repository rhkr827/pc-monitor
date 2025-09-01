import { SystemMonitor } from './system-monitor';
import { WebServer } from './web-server';

async function main(): Promise<void> {
  try {
    console.log('PC Monitor (TypeScript) - Starting...');

    // Initialize system monitor
    const monitor = new SystemMonitor();
    const initialized = await monitor.initialize();
    
    if (!initialized) {
      console.error('Failed to initialize system monitor');
      process.exit(1);
    }

    console.log('✅ System monitor initialized');

    // Test system stats
    const stats = await monitor.getSystemStats();
    console.log(`🖥️  CPU Usage: ${stats.cpu.overall.toFixed(1)}%`);
    console.log(`💾 Memory Usage: ${(stats.memory.used / 1024 / 1024 / 1024).toFixed(1)}GB (${stats.memory.usagePercent.toFixed(1)}%)`);
    console.log(`🔥 CPU Cores: ${stats.cpu.cores.length} detected`);

    // Start web server
    const PORT = 3002;
    const server = new WebServer(monitor, PORT);
    
    const serverStarted = await server.start();
    if (!serverStarted) {
      console.error('Failed to start web server');
      process.exit(1);
    }

    console.log(`🚀 Server running on http://localhost:${PORT}`);
    console.log('Available endpoints:');
    console.log('  • GET /api/stats   - Complete system stats');
    console.log('  • GET /api/cpu     - CPU usage data');
    console.log('  • GET /api/memory  - Memory usage data');
    console.log('  • GET /health      - Health check');
    console.log('  • WS  /ws/stats    - WebSocket stats stream');
    console.log('\nPress Ctrl+C to stop...\n');

    // Handle graceful shutdown
    process.on('SIGINT', () => {
      console.log('\nReceived SIGINT, shutting down gracefully...');
      server.stop();
      process.exit(0);
    });

    process.on('SIGTERM', () => {
      console.log('\nReceived SIGTERM, shutting down gracefully...');
      server.stop();
      process.exit(0);
    });

    // Keep the process alive
    process.stdin.resume();

  } catch (error) {
    console.error('❌ Error:', error);
    process.exit(1);
  }
}

// Handle unhandled promise rejections
process.on('unhandledRejection', (reason, promise) => {
  console.error('Unhandled Rejection at:', promise, 'reason:', reason);
});

// Handle uncaught exceptions
process.on('uncaughtException', (error) => {
  console.error('Uncaught Exception:', error);
  process.exit(1);
});

main();