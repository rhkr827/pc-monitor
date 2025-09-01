#pragma once

#include "system_monitor.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <set>

namespace pc_monitor {

class WebServer {
public:
    explicit WebServer(std::shared_ptr<SystemMonitor> monitor, std::uint16_t port = 3001);
    ~WebServer();
    
    // Disable copy, enable move
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    WebServer(WebServer&&) noexcept = default;
    WebServer& operator=(WebServer&&) noexcept = default;
    
    Result<void> start();
    void stop();
    bool isRunning() const noexcept { return running.load(); }
    
private:
    void setupRoutes();
    void setupCORS();
    void handleCPUEndpoint(const httplib::Request& req, httplib::Response& res);
    void handleMemoryEndpoint(const httplib::Request& req, httplib::Response& res);
    void handleStatsEndpoint(const httplib::Request& req, httplib::Response& res);
    
    // WebSocket support
    void handleWebSocket(const httplib::Request& req, httplib::Response& res);
    void broadcastStats();
    void startBroadcastThread();
    
    std::shared_ptr<SystemMonitor> monitor_;
    std::unique_ptr<httplib::Server> server_;
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
    nlohmann::json to_json(const CPUCoreData& core);
    nlohmann::json to_json(const CPUUsageData& cpu);
    nlohmann::json to_json(const MemoryUsageData& memory);
    nlohmann::json to_json(const SystemStats& stats);
    
    template<typename T>
    nlohmann::json error_response(SystemError error, std::string_view message) {
        return nlohmann::json{
            {"error", true},
            {"code", static_cast<int>(error)},
            {"message", message},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        };
    }
}

} // namespace pc_monitor