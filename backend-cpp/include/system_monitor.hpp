#pragma once

#include <chrono>
#include <coroutine>
#include <expected>
#include <memory>
#include <ranges>
#include <vector>
#include <format>
#include <span>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#endif

namespace pc_monitor {

// Modern C++23 error handling
enum class SystemError {
    PERMISSION_DENIED,
    SYSTEM_ERROR,
    INITIALIZATION_FAILED,
    DATA_UNAVAILABLE
};

template<typename T>
using Result = std::expected<T, SystemError>;

// Core data structures using C++23 features
struct CPUCoreData {
    std::uint32_t coreId;
    double usage;       // 0-100%
    std::uint64_t frequency; // MHz
    
    auto operator<=>(const CPUCoreData&) const = default;
};

struct CPUUsageData {
    double overall;              // 0-100%
    std::optional<double> temperature; // Celsius
    std::uint64_t averageFrequency;    // MHz
    std::vector<CPUCoreData> cores;
    
    auto operator<=>(const CPUUsageData&) const = default;
};

struct MemoryUsageData {
    std::uint64_t total;        // bytes
    std::uint64_t used;         // bytes  
    std::uint64_t available;    // bytes
    std::uint64_t cache;        // bytes
    std::uint64_t buffers;      // bytes
    double usagePercent;        // calculated 0-100%
    
    auto operator<=>(const MemoryUsageData&) const = default;
};

struct SystemStats {
    CPUUsageData cpu;
    MemoryUsageData memory;
    std::chrono::system_clock::time_point timestamp;
    
    auto operator<=>(const SystemStats&) const = default;
};

// Modern C++23 coroutine-based system monitor
class SystemMonitor {
public:
    explicit SystemMonitor();
    ~SystemMonitor();
    
    // Disable copy, enable move
    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;
    SystemMonitor(SystemMonitor&&) noexcept = default;
    SystemMonitor& operator=(SystemMonitor&&) noexcept = default;
    
    Result<void> initialize();
    Result<SystemStats> getCurrentStats();
    
    // C++23 coroutine generator for streaming data
    struct StatsGenerator {
        struct promise_type {
            SystemStats current_value;
            
            StatsGenerator get_return_object() {
                return StatsGenerator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            
            std::suspend_always yield_value(SystemStats stats) {
                current_value = std::move(stats);
                return {};
            }
            
            void return_void() {}
            void unhandled_exception() { std::terminate(); }
        };
        
        std::coroutine_handle<promise_type> coro;
        
        explicit StatsGenerator(std::coroutine_handle<promise_type> h) : coro(h) {}
        
        ~StatsGenerator() {
            if (coro) coro.destroy();
        }
        
        StatsGenerator(const StatsGenerator&) = delete;
        StatsGenerator& operator=(const StatsGenerator&) = delete;
        
        StatsGenerator(StatsGenerator&& other) noexcept : coro(std::exchange(other.coro, {})) {}
        StatsGenerator& operator=(StatsGenerator&& other) noexcept {
            if (this != &other) {
                if (coro) coro.destroy();
                coro = std::exchange(other.coro, {});
            }
            return *this;
        }
        
        bool move_next() {
            coro.resume();
            return !coro.done();
        }
        
        SystemStats current_value() const {
            return coro.promise().current_value;
        }
    };
    
    StatsGenerator streamStats(std::chrono::milliseconds interval = std::chrono::milliseconds{1000});

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Utility functions using C++23 ranges and formatting
namespace utils {
    template<std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, double>
    constexpr double average(R&& range) {
        if (std::ranges::empty(range)) return 0.0;
        
        auto sum = std::ranges::fold_left(range, 0.0, std::plus{});
        return sum / static_cast<double>(std::ranges::size(range));
    }
    
    constexpr std::string formatBytes(std::uint64_t bytes) {
        constexpr std::array<std::string_view, 5> units = {"B", "KB", "MB", "GB", "TB"};
        
        if (bytes == 0) return "0 B";
        
        auto unitIndex = static_cast<std::size_t>(
            std::floor(std::log2(bytes) / 10.0)
        );
        unitIndex = std::min(unitIndex, units.size() - 1);
        
        double value = static_cast<double>(bytes) / std::pow(1024, unitIndex);
        return std::format("{:.2f} {}", value, units[unitIndex]);
    }
    
    constexpr std::string formatPercentage(double percent) {
        return std::format("{:.1f}%", std::clamp(percent, 0.0, 100.0));
    }
}

} // namespace pc_monitor