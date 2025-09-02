#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <coroutine>
#include <expected>
#include <format>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <vector>

namespace pc_monitor {

// Modern C++23 error handling
enum class SystemError : std::uint8_t { PERMISSION_DENIED, SYSTEM_ERROR, INITIALIZATION_FAILED, DATA_UNAVAILABLE };

template <typename T>
using Result = std::expected<T, SystemError>;

// Core data structures using C++23 features
struct CPUCoreData {
    std::uint32_t coreId;
    double usage;             // 0-100%
    std::uint64_t frequency;  // MHz

    auto operator<=>(const CPUCoreData&) const = default;
};

struct CPUUsageData {
    double overall{};                   // 0-100%
    std::optional<double> temperature;  // Celsius
    std::uint64_t averageFrequency{};   // MHz
    std::vector<CPUCoreData> cores{};

    auto operator<=>(const CPUUsageData&) const = default;
};

struct MemoryUsageData {
    std::uint64_t total;      // bytes
    std::uint64_t used;       // bytes
    std::uint64_t available;  // bytes
    std::uint64_t cache;      // bytes
    std::uint64_t buffers;    // bytes
    double usagePercent;      // calculated 0-100%

    auto operator<=>(const MemoryUsageData&) const = default;
};

struct SystemStats {
    CPUUsageData cpu;
    MemoryUsageData memory{};
    std::chrono::system_clock::time_point timestamp{};

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

    Result<void> Initialize();
    Result<SystemStats> GetCurrentStats();

    // C++23 coroutine generator for streaming data
    struct StatsGenerator {
        struct PromiseType {
            SystemStats current_value;

            StatsGenerator get_return_object() {
                return StatsGenerator{std::coroutine_handle<PromiseType>::from_promise(*this)};
            }

            static std::suspend_always initial_suspend() noexcept {
                return {};
            }
            static std::suspend_always final_suspend() noexcept {
                return {};
            }

            std::suspend_always yield_value(SystemStats stats) {
                current_value = std::move(stats);
                return {};
            }

            void return_void() {}
            static void unhandled_exception() {
                std::terminate();
            }
        };

        using promise_type = PromiseType;
        std::coroutine_handle<PromiseType> coro;

        explicit StatsGenerator(std::coroutine_handle<PromiseType> h) : coro(h) {}

        ~StatsGenerator() {
            if (coro) {
                coro.destroy();
            }
        }

        StatsGenerator(const StatsGenerator&) = delete;
        StatsGenerator& operator=(const StatsGenerator&) = delete;

        StatsGenerator(StatsGenerator&& other) noexcept : coro(std::exchange(other.coro, {})) {}
        StatsGenerator& operator=(StatsGenerator&& other) noexcept {
            if (this != &other) {
                if (coro) {
                    coro.destroy();
                }
                coro = std::exchange(other.coro, {});
            }
            return *this;
        }

        [[nodiscard]] bool move_next() const {
            coro.resume();
            return !coro.done();
        }

        [[nodiscard]] SystemStats current_value() const {
            return coro.promise().current_value;
        }
    };

    StatsGenerator StreamStats(std::chrono::milliseconds interval = std::chrono::milliseconds{1000});

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

// Utility functions using C++23 ranges and formatting
namespace utils {
template <std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, double>
constexpr double Average(R&& range) {
    if (std::ranges::empty(range)) {
        return 0.0;
    }

    // Use std::accumulate instead of std::ranges::fold_left for MSVC compatibility
    auto Sum = std::accumulate(std::ranges::begin(range), std::ranges::end(range), 0.0);
    return Sum / static_cast<double>(std::ranges::size(range));
}

inline std::string FormatBytes(std::uint64_t bytes) {
    constexpr std::array<std::string_view, 5> Units = {"B", "KB", "MB", "GB", "TB"};

    if (bytes == 0) {
        return "0 B";
    }

    auto UnitIndex = static_cast<std::size_t>(std::floor(std::log2(bytes) / 10.0));
    UnitIndex = std::min(UnitIndex, Units.size() - 1);

    double Value = static_cast<double>(bytes) / std::pow(1024, UnitIndex);
    return std::format("{:.2f} {}", Value, Units[UnitIndex]);
}

inline std::string FormatPercentage(double percent) {
    return std::format("{:.1f}%", std::clamp(percent, 0.0, 100.0));
}
}  // namespace utils

}  // namespace pc_monitor