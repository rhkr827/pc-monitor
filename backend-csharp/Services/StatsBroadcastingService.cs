using Microsoft.AspNetCore.SignalR;
using PCMonitor.Hubs;
using PCMonitor.Models;

namespace PCMonitor.Services;

public class StatsBroadcastingService : BackgroundService
{
    private readonly IHubContext<SystemStatsHub> _hubContext;
    private readonly ISystemMonitorService _systemMonitor;
    private readonly TimeSpan _broadcastInterval = TimeSpan.FromSeconds(1);

    public StatsBroadcastingService(
        IHubContext<SystemStatsHub> hubContext,
        ISystemMonitorService systemMonitor)
    {
        _hubContext = hubContext;
        _systemMonitor = systemMonitor;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        // Wait for system monitor to initialize
        while (!_systemMonitor.IsInitialized && !stoppingToken.IsCancellationRequested)
        {
            await Task.Delay(1000, stoppingToken);
        }

        if (!_systemMonitor.IsInitialized)
            return;

        using var timer = new PeriodicTimer(_broadcastInterval);

        while (!stoppingToken.IsCancellationRequested && 
               await timer.WaitForNextTickAsync(stoppingToken))
        {
            try
            {
                var stats = await _systemMonitor.GetSystemStatsAsync();
                
                var message = new WebSocketMessage
                {
                    Type = "stats",
                    Timestamp = stats.Timestamp,
                    Data = stats
                };

                await _hubContext.Clients.All.SendAsync("ReceiveStats", message, stoppingToken);
            }
            catch (Exception ex)
            {
                // Log error but continue broadcasting
                Console.WriteLine($"Broadcasting error: {ex.Message}");
            }
        }
    }
}