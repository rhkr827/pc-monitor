using System.Text.Json.Serialization;

namespace PCMonitor.Models;

public record CPUCoreData
{
    [JsonPropertyName("coreId")]
    public required uint CoreId { get; init; }
    
    [JsonPropertyName("usage")]
    public required double Usage { get; init; } // 0-100%
    
    [JsonPropertyName("frequency")]
    public required ulong Frequency { get; init; } // MHz
}

public record CPUUsageData
{
    [JsonPropertyName("overall")]
    public required double Overall { get; init; } // 0-100%
    
    [JsonPropertyName("temperature")]
    public double? Temperature { get; init; } // Celsius
    
    [JsonPropertyName("averageFrequency")]
    public required ulong AverageFrequency { get; init; } // MHz
    
    [JsonPropertyName("cores")]
    public required IReadOnlyList<CPUCoreData> Cores { get; init; }
}

public record MemoryUsageData
{
    [JsonPropertyName("total")]
    public required ulong Total { get; init; } // bytes
    
    [JsonPropertyName("used")]
    public required ulong Used { get; init; } // bytes
    
    [JsonPropertyName("available")]
    public required ulong Available { get; init; } // bytes
    
    [JsonPropertyName("cache")]
    public required ulong Cache { get; init; } // bytes
    
    [JsonPropertyName("buffers")]
    public required ulong Buffers { get; init; } // bytes
    
    [JsonPropertyName("usagePercent")]
    public required double UsagePercent { get; init; } // 0-100%
}

public record SystemStats
{
    [JsonPropertyName("cpu")]
    public required CPUUsageData CPU { get; init; }
    
    [JsonPropertyName("memory")]
    public required MemoryUsageData Memory { get; init; }
    
    [JsonPropertyName("timestamp")]
    public required long Timestamp { get; init; }
}

public record ErrorResponse
{
    [JsonPropertyName("error")]
    public bool Error { get; init; } = true;
    
    [JsonPropertyName("code")]
    public required string Code { get; init; }
    
    [JsonPropertyName("message")]
    public required string Message { get; init; }
    
    [JsonPropertyName("timestamp")]
    public long Timestamp { get; init; } = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
}

public record WebSocketMessage
{
    [JsonPropertyName("type")]
    public required string Type { get; init; }
    
    [JsonPropertyName("timestamp")]
    public long Timestamp { get; init; } = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
    
    [JsonPropertyName("data")]
    public object? Data { get; init; }
}