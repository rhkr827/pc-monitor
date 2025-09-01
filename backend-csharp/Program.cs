using PCMonitor.Hubs;
using PCMonitor.Services;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllers();
builder.Services.AddSignalR();

builder.Services.AddSingleton<ISystemMonitorService, SystemMonitorService>();
builder.Services.AddHostedService<StatsBroadcastingService>();

builder.Services.AddCors(options =>
{
    options.AddDefaultPolicy(policy =>
    {
        policy.AllowAnyOrigin()
              .AllowAnyMethod() 
              .AllowAnyHeader();
    });
});

var app = builder.Build();

app.UseCors();
app.UseRouting();

app.MapGet("/health", () => Results.Ok(new { status = "ok", timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() }));

app.MapGet("/api/cpu", async (ISystemMonitorService monitor) =>
{
    try
    {
        var cpuData = await monitor.GetCPUDataAsync();
        return Results.Ok(cpuData);
    }
    catch (Exception ex)
    {
        return Results.Problem($"Failed to get CPU data: {ex.Message}");
    }
});

app.MapGet("/api/memory", async (ISystemMonitorService monitor) =>
{
    try
    {
        var memoryData = await monitor.GetMemoryDataAsync();
        return Results.Ok(memoryData);
    }
    catch (Exception ex)
    {
        return Results.Problem($"Failed to get memory data: {ex.Message}");
    }
});

app.MapGet("/api/stats", async (ISystemMonitorService monitor) =>
{
    try
    {
        var stats = await monitor.GetSystemStatsAsync();
        return Results.Ok(stats);
    }
    catch (Exception ex)
    {
        return Results.Problem($"Failed to get system stats: {ex.Message}");
    }
});

app.MapHub<SystemStatsHub>("/ws/stats");

// Initialize system monitor
var monitor = app.Services.GetRequiredService<ISystemMonitorService>();
await monitor.InitializeAsync();

const int PORT = 3003;
app.Urls.Add($"http://localhost:{PORT}");

Console.WriteLine("PC Monitor (C# .NET 9.0) - Starting...");
Console.WriteLine("✅ System monitor initialized");

var testStats = await monitor.GetSystemStatsAsync();
Console.WriteLine($"🖥️  CPU Usage: {testStats.CPU.Overall:F1}%");
Console.WriteLine($"💾 Memory Usage: {testStats.Memory.Used / 1024.0 / 1024.0 / 1024.0:F1}GB ({testStats.Memory.UsagePercent:F1}%)");
Console.WriteLine($"🔥 CPU Cores: {testStats.CPU.Cores.Count} detected");

Console.WriteLine($"🚀 Server running on http://localhost:{PORT}");
Console.WriteLine("Available endpoints:");
Console.WriteLine("  • GET /api/stats   - Complete system stats");
Console.WriteLine("  • GET /api/cpu     - CPU usage data");
Console.WriteLine("  • GET /api/memory  - Memory usage data");  
Console.WriteLine("  • GET /health      - Health check");
Console.WriteLine("  • SignalR /ws/stats - Real-time stats hub");
Console.WriteLine("\nPress Ctrl+C to stop...\n");

await app.RunAsync();