#include "web_server.hpp"
#include <format>
#include <ranges>

namespace pc_monitor {

WebServer::WebServer(std::shared_ptr<SystemMonitor> monitor, std::uint16_t port)
    : monitor_(std::move(monitor))
    , server_(std::make_unique<httplib::Server>())
    , port_(port) {
    setupCORS();
    setupRoutes();
}

WebServer::~WebServer() {
    stop();
}

Result<void> WebServer::start() {
    if (running.load()) {
        return std::unexpected(SystemError::SYSTEM_ERROR);
    }
    
    startBroadcastThread();
    
    // Start server in a separate thread
    std::thread serverThread([this]() {
        running.store(true);
        server_->listen("localhost", port_);
    });
    
    serverThread.detach();
    
    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    
    return {};
}

void WebServer::stop() {
    if (running.load()) {
        running.store(false);
        shouldBroadcast_.store(false);
        
        if (server_) {
            server_->stop();
        }
        
        if (broadcastThread_.joinable()) {
            broadcastThread_.join();
        }
    }
}

void WebServer::setupCORS() {
    server_->set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
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

void WebServer::setupRoutes() {
    // API routes
    server_->Get("/api/cpu", [this](const httplib::Request& req, httplib::Response& res) {
        handleCPUEndpoint(req, res);
    });
    
    server_->Get("/api/memory", [this](const httplib::Request& req, httplib::Response& res) {
        handleMemoryEndpoint(req, res);
    });
    
    server_->Get("/api/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleStatsEndpoint(req, res);
    });
    
    // Health check
    server_->Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","service":"pc-monitor-cpp"})", "application/json");
    });
    
    // WebSocket endpoint (simplified - httplib has limited WebSocket support)
    server_->Get("/ws/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handleWebSocket(req, res);
    });
}

void WebServer::handleCPUEndpoint(const httplib::Request&, httplib::Response& res) {
    auto stats = monitor_->getCurrentStats();
    if (!stats) {
        res.status = 500;
        res.set_content(
            json::error_response(stats.error(), "Failed to get CPU stats").dump(),
            "application/json"
        );
        return;
    }
    
    res.set_content(json::to_json(stats->cpu).dump(), "application/json");
}

void WebServer::handleMemoryEndpoint(const httplib::Request&, httplib::Response& res) {
    auto stats = monitor_->getCurrentStats();
    if (!stats) {
        res.status = 500;
        res.set_content(
            json::error_response(stats.error(), "Failed to get memory stats").dump(),
            "application/json"
        );
        return;
    }
    
    res.set_content(json::to_json(stats->memory).dump(), "application/json");
}

void WebServer::handleStatsEndpoint(const httplib::Request&, httplib::Response& res) {
    auto stats = monitor_->getCurrentStats();
    if (!stats) {
        res.status = 500;
        res.set_content(
            json::error_response(stats.error(), "Failed to get system stats").dump(),
            "application/json"
        );
        return;
    }
    
    res.set_content(json::to_json(*stats).dump(), "application/json");
}

void WebServer::handleWebSocket(const httplib::Request& req, httplib::Response& res) {
    // Simplified WebSocket handling - in a real implementation,
    // you would use a proper WebSocket library
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    
    // Send stats every second
    for (int i = 0; i < 60 && running.load(); ++i) {
        auto stats = monitor_->getCurrentStats();
        if (stats) {
            auto json_data = json::to_json(*stats);
            std::string sse_data = std::format("data: {}\\n\\n", json_data.dump());
            res.set_content(sse_data, "text/plain");
        }
        
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
}

void WebServer::startBroadcastThread() {
    shouldBroadcast_.store(true);
    broadcastThread_ = std::thread([this]() {
        while (shouldBroadcast_.load()) {
            broadcastStats();
            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
    });
}

void WebServer::broadcastStats() {
    auto stats = monitor_->getCurrentStats();
    if (!stats) return;
    
    auto json_data = json::to_json(*stats);
    std::string message = json_data.dump();
    
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    // Clean up expired clients and broadcast to active ones
    for (auto it = wsClients_.begin(); it != wsClients_.end();) {
        if (auto client = it->lock()) {
            // In a real WebSocket implementation, we would send the message here
            ++it;
        } else {
            it = wsClients_.erase(it);
        }
    }
}

// JSON serialization implementations
namespace json {
    nlohmann::json to_json(const CPUCoreData& core) {
        return nlohmann::json{
            {"coreId", core.coreId},
            {"usage", core.usage},
            {"frequency", core.frequency}
        };
    }
    
    nlohmann::json to_json(const CPUUsageData& cpu) {
        nlohmann::json cores_json = nlohmann::json::array();
        
        // Use C++23 ranges for transformation
        auto core_jsons = cpu.cores 
            | std::views::transform([](const auto& core) { return to_json(core); });
        
        for (const auto& core_json : core_jsons) {
            cores_json.push_back(core_json);
        }
        
        nlohmann::json result{
            {"overall", cpu.overall},
            {"averageFrequency", cpu.averageFrequency},
            {"cores", std::move(cores_json)}
        };
        
        if (cpu.temperature.has_value()) {
            result["temperature"] = *cpu.temperature;
        }
        
        return result;
    }
    
    nlohmann::json to_json(const MemoryUsageData& memory) {
        return nlohmann::json{
            {"total", memory.total},
            {"used", memory.used},
            {"available", memory.available},
            {"cache", memory.cache},
            {"buffers", memory.buffers},
            {"usagePercent", memory.usagePercent}
        };
    }
    
    nlohmann::json to_json(const SystemStats& stats) {
        auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            stats.timestamp.time_since_epoch()).count();
        
        return nlohmann::json{
            {"cpu", to_json(stats.cpu)},
            {"memory", to_json(stats.memory)},
            {"timestamp", timestamp_ms}
        };
    }
}

} // namespace pc_monitor