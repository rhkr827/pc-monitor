#pragma once

// Standard library includes first
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string_view>
#include <thread>

// Third-party includes
#include <httplib.h>
#include <nlohmann/json.hpp>

// Local includes last
#include "system_monitor.hpp"

namespace pc_monitor {

class WebServer {
public:
    explicit WebServer(std::shared_ptr<SystemMonitor> monitor, std::uint16_t port = 3001);
    ~WebServer();

    // Disable copy and move (due to atomic members)
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    WebServer(WebServer&&) = delete;
    WebServer& operator=(WebServer&&) = delete;

    Result<void> Start();
    void Stop();
    [[nodiscard]] bool IsRunning() const noexcept {
        return running_.load();
    }

private:
    void SetupRoutes();
    void SetupCors();
    void HandleCpuEndpoint(const httplib::Request& req, httplib::Response& res);
    void HandleMemoryEndpoint(const httplib::Request& req, httplib::Response& res);
    void HandleStatsEndpoint(const httplib::Request& req, httplib::Response& res);

    // WebSocket support
    void HandleWebSocket(const httplib::Request& req, httplib::Response& res);
    void BroadcastStats();
    void StartBroadcastThread();

    std::shared_ptr<SystemMonitor> monitor_;
    std::unique_ptr<httplib::Server> server_{};
    std::uint16_t port_;
    std::atomic<bool> running_{false};

    // WebSocket clients management
    std::mutex clientsMutex_;
    std::set<std::weak_ptr<httplib::Response>, std::owner_less<std::weak_ptr<httplib::Response>>> wsClients_;
    std::thread broadcastThread_;
    std::atomic<bool> shouldBroadcast_{false};
};

// JSON serialization functions using C++23 features
namespace json {
nlohmann::json ToJson(const CPUCoreData& core);
nlohmann::json ToJson(const CPUUsageData& cpu);
nlohmann::json ToJson(const MemoryUsageData& memory);
nlohmann::json ToJson(const SystemStats& stats);

inline nlohmann::json ErrorResponse(SystemError error, std::string_view message) {
    return nlohmann::json{
        {"error", true},
        {"code", static_cast<int>(error)},
        {"message", message},
        {"timestamp",
         std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
             .count()}};
}
}  // namespace json

}  // namespace pc_monitor