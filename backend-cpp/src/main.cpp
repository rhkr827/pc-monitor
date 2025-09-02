#include "system_monitor.hpp"
#include "web_server.hpp"

#include <atomic>
#include <csignal>
#include <format>
#include <iostream>

namespace {
std::atomic<bool> should_exit{false};

void signal_handler(int signal) {
    std::cout << std::format("\\nReceived signal {}, shutting down gracefully...\\n", signal);
    should_exit.store(true);
}
}  // namespace

int main() {
    try {
        // Set up signal handling
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        std::cout << "PC Monitor (C++23) - Starting...\\n";

        // Initialize system monitor
        auto monitor = std::make_shared<pc_monitor::SystemMonitor>();

        auto init_result = monitor->Initialize();
        if (!init_result) {
            std::cerr << "Failed to initialize system monitor\\n";
            return 1;
        }

        std::cout << "✅ System monitor initialized\\n";

        // Test system stats
        auto stats = monitor->GetCurrentStats();
        if (stats) {
            std::cout << std::format("🖥️  CPU Usage: {}\\n", pc_monitor::utils::FormatPercentage(stats->cpu.overall));
            std::cout << std::format("💾 Memory Usage: {} ({})\\n",
                                     pc_monitor::utils::FormatBytes(stats->memory.used),
                                     pc_monitor::utils::FormatPercentage(stats->memory.usagePercent));
            std::cout << std::format("🔥 CPU Cores: {} detected\\n", stats->cpu.cores.size());
        }

        // Start web server
        constexpr std::uint16_t PORT = 3001;
        auto server = std::make_unique<pc_monitor::WebServer>(monitor, PORT);

        auto server_result = server->Start();
        if (!server_result) {
            std::cerr << "Failed to start web server\\n";
            return 1;
        }

        std::cout << std::format("🚀 Server running on http://localhost:{}\\n", PORT);
        std::cout << "Available endpoints:\\n";
        std::cout << "  • GET /api/stats   - Complete system stats\\n";
        std::cout << "  • GET /api/cpu     - CPU usage data\\n";
        std::cout << "  • GET /api/memory  - Memory usage data\\n";
        std::cout << "  • GET /health      - Health check\\n";
        std::cout << "  • GET /ws/stats    - WebSocket/SSE stats stream\\n";
        std::cout << R"(\nPress Ctrl+C to stop...\n\n)";

        // Main loop - demonstrate C++23 coroutine usage
        auto stats_stream = monitor->StreamStats(std::chrono::seconds{5});

        while (!should_exit.load() && server->IsRunning()) {
            if (stats_stream.move_next()) {
                const auto current_stats = stats_stream.current_value();

                // Display real-time stats using C++23 formatting
                std::cout << std::format("⏱️  [{}] CPU: {} | Memory: {} | Cores: {}\r",
                                         std::chrono::duration_cast<std::chrono::seconds>(
                                             std::chrono::system_clock::now().time_since_epoch())
                                                 .count() %
                                             60,
                                         pc_monitor::utils::FormatPercentage(current_stats.cpu.overall),
                                         pc_monitor::utils::FormatPercentage(current_stats.memory.usagePercent),
                                         current_stats.cpu.cores.size());
                std::cout.flush();
            }

            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }

        std::cout << "\\n🛑 Shutting down server...\\n";
        server->Stop();
        std::cout << "✅ Shutdown complete\\n";

    } catch (const std::exception& e) {
        std::cerr << std::format("❌ Error: {}\\n", e.what());
        return 1;
    }

    return 0;
}