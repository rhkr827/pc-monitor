#include "web_server.hpp"

#include <format>
#include <ranges>

namespace pc_monitor {

WebServer::WebServer(std::shared_ptr<SystemMonitor> monitor, std::uint16_t port)
    : monitor_(std::move(monitor)), server_(std::make_unique<httplib::Server>()), port_(port) {
    SetupCors();
    SetupRoutes();
}

WebServer::~WebServer() {
    Stop();
}

Result<void> WebServer::Start() {
    if (running_.load()) {
        return std::unexpected(SystemError::SYSTEM_ERROR);
    }

    StartBroadcastThread();

    // Start server in a separate thread
    std::thread ServerThread([this]() {
        running_.store(true);
        server_->listen("localhost", port_);
    });

    ServerThread.detach();

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    return {};
}

void WebServer::Stop() {
    if (running_.load()) {
        running_.store(false);
        shouldBroadcast_.store(false);

        if (server_) {
            server_->stop();
        }

        if (broadcastThread_.joinable()) {
            broadcastThread_.join();
        }
    }
}

void WebServer::SetupCors() {
    server_->set_pre_routing_handler([](const httplib::Request& /* req */, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Handle preflight requests
    server_->Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
        return;
    });
}

void WebServer::SetupRoutes() {
    // API routes
    server_->Get("/api/cpu",
                 [this](const httplib::Request& req, httplib::Response& res) { HandleCpuEndpoint(req, res); });

    server_->Get("/api/memory",
                 [this](const httplib::Request& req, httplib::Response& res) { HandleMemoryEndpoint(req, res); });

    server_->Get("/api/stats",
                 [this](const httplib::Request& req, httplib::Response& res) { HandleStatsEndpoint(req, res); });

    // Health check
    server_->Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","service":"pc-monitor-cpp"})", "application/json");
    });

    // WebSocket endpoint (simplified - httplib has limited WebSocket support)
    server_->Get("/ws/stats",
                 [this](const httplib::Request& req, httplib::Response& res) { HandleWebSocket(req, res); });
}

void WebServer::HandleCpuEndpoint(const httplib::Request& /*unused*/, httplib::Response& res) {
    auto Stats = monitor_->GetCurrentStats();
    if (!Stats) {
        res.status = 500;
        res.set_content(json::ErrorResponse(Stats.error(), "Failed to get CPU stats").dump(), "application/json");
        return;
    }

    res.set_content(json::ToJson(Stats->cpu).dump(), "application/json");
}

void WebServer::HandleMemoryEndpoint(const httplib::Request& /*unused*/, httplib::Response& res) {
    auto Stats = monitor_->GetCurrentStats();
    if (!Stats) {
        res.status = 500;
        res.set_content(json::ErrorResponse(Stats.error(), "Failed to get memory stats").dump(), "application/json");
        return;
    }

    res.set_content(json::ToJson(Stats->memory).dump(), "application/json");
}

void WebServer::HandleStatsEndpoint(const httplib::Request& /*unused*/, httplib::Response& res) {
    auto Stats = monitor_->GetCurrentStats();
    if (!Stats) {
        res.status = 500;
        res.set_content(json::ErrorResponse(Stats.error(), "Failed to get system stats").dump(), "application/json");
        return;
    }

    res.set_content(json::ToJson(*Stats).dump(), "application/json");
}

void WebServer::HandleWebSocket(const httplib::Request& /* req */, httplib::Response& res) {
    // Simplified WebSocket handling - in a real implementation,
    // you would use a proper WebSocket library
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");

    // Send stats every second
    for (int I = 0; I < 60 && running_.load(); ++I) {
        auto Stats = monitor_->GetCurrentStats();
        if (Stats) {
            auto JsonData = json::ToJson(*Stats);
            std::string SseData = std::format("data: {}\n\n", JsonData.dump());
            res.set_content(SseData, "text/plain");
        }

        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
}

void WebServer::StartBroadcastThread() {
    shouldBroadcast_.store(true);
    broadcastThread_ = std::thread([this]() {
        while (shouldBroadcast_.load()) {
            BroadcastStats();
            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
    });
}

void WebServer::BroadcastStats() {
    auto Stats = monitor_->GetCurrentStats();
    if (!Stats) {
        return;
    }

    auto JsonData = json::ToJson(*Stats);
    std::string Message = JsonData.dump();

    std::lock_guard<std::mutex> const Lock(clientsMutex_);

    // Clean up expired clients and broadcast to active ones
    for (auto It = wsClients_.begin(); It != wsClients_.end();) {
        if (auto client = It->lock()) {
            // In a real WebSocket implementation, we would send the message here
            ++It;
        } else {
            It = wsClients_.erase(It);
        }
    }
}

// JSON serialization implementations
namespace json {
nlohmann::json ToJson(const CPUCoreData& core) {
    return nlohmann::json{{"coreId", core.coreId}, {"usage", core.usage}, {"frequency", core.frequency}};
}

nlohmann::json ToJson(const CPUUsageData& cpu) {
    nlohmann::json CoresJson = nlohmann::json::array();

    // Use C++23 ranges for transformation
    auto CoreJsons = cpu.cores | std::views::transform([](const auto& core) { return ToJson(core); });

    for (const auto& CoreJson : CoreJsons) {
        CoresJson.push_back(CoreJson);
    }

    nlohmann::json Result{
        {"overall", cpu.overall}, {"averageFrequency", cpu.averageFrequency}, {"cores", std::move(CoresJson)}};

    if (cpu.temperature.has_value()) {
        Result["temperature"] = *cpu.temperature;
    }

    return Result;
}

nlohmann::json ToJson(const MemoryUsageData& memory) {
    return nlohmann::json{{"total", memory.total},
                          {"used", memory.used},
                          {"available", memory.available},
                          {"cache", memory.cache},
                          {"buffers", memory.buffers},
                          {"usagePercent", memory.usagePercent}};
}

nlohmann::json ToJson(const SystemStats& stats) {
    auto TimestampMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(stats.timestamp.time_since_epoch()).count();

    return nlohmann::json{{"cpu", ToJson(stats.cpu)}, {"memory", ToJson(stats.memory)}, {"timestamp", TimestampMs}};
}
}  // namespace json

}  // namespace pc_monitor