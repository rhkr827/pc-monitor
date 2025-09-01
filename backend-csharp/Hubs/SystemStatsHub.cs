using Microsoft.AspNetCore.SignalR;
using PCMonitor.Services;

namespace PCMonitor.Hubs;

public class SystemStatsHub : Hub
{
    private readonly ISystemMonitorService _systemMonitor;

    public SystemStatsHub(ISystemMonitorService systemMonitor)
    {
        _systemMonitor = systemMonitor;
    }

    public async Task JoinGroup(string groupName = "default")
    {
        await Groups.AddToGroupAsync(Context.ConnectionId, groupName);
        await Clients.Caller.SendAsync("Joined", $"Joined group {groupName}");
    }

    public async Task LeaveGroup(string groupName = "default")
    {
        await Groups.RemoveFromGroupAsync(Context.ConnectionId, groupName);
        await Clients.Caller.SendAsync("Left", $"Left group {groupName}");
    }

    public async Task GetCurrentStats()
    {
        try
        {
            var stats = await _systemMonitor.GetSystemStatsAsync();
            await Clients.Caller.SendAsync("StatsUpdate", stats);
        }
        catch (Exception ex)
        {
            await Clients.Caller.SendAsync("Error", new { message = ex.Message });
        }
    }
}