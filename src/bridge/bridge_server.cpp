/**
 * @file bridge_server.cpp
 * @brief WebSocket bridge server implementation
 */

#include "kinect_xr/bridge_server.h"
#include "kinect_xr/device.h"

#include <ixwebsocket/IXWebSocketServer.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstring>
#include <iostream>

using json = nlohmann::json;

namespace kinect_xr {

namespace {
constexpr int FRAME_INTERVAL_MS = 33;  // ~30 Hz
constexpr const char* PROTOCOL_VERSION = "1.0";
constexpr const char* SERVER_NAME = "kinect-xr-bridge";
}  // namespace

BridgeServer::BridgeServer() = default;

BridgeServer::~BridgeServer() {
    stop();
}

bool BridgeServer::start(int port) {
    if (running_) {
        return false;
    }

    port_ = port;

    // Create WebSocket server
    server_ = std::make_unique<ix::WebSocketServer>(port, "0.0.0.0");

    // Set up connection handler
    server_->setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> connectionState,
               ix::WebSocket& webSocket,
               const ix::WebSocketMessagePtr& msg) {
            // Store raw pointer for client tracking (safe within callback scope)
            ix::WebSocket* wsPtr = &webSocket;

            switch (msg->type) {
                case ix::WebSocketMessageType::Open:
                    onConnection(wsPtr);
                    break;
                case ix::WebSocketMessageType::Close:
                    onClose(wsPtr);
                    break;
                case ix::WebSocketMessageType::Message:
                    if (!msg->binary) {
                        onMessage(wsPtr, msg->str);
                    }
                    break;
                case ix::WebSocketMessageType::Error:
                    std::cerr << "WebSocket error: " << msg->errorInfo.reason << std::endl;
                    break;
                default:
                    break;
            }
        });

    // Start listening
    auto res = server_->listen();
    if (!res.first) {
        std::cerr << "Failed to start server: " << res.second << std::endl;
        return false;
    }

    // Start server in background
    server_->start();
    running_ = true;

    // Start broadcast loop
    broadcastRunning_ = true;
    broadcastThread_ = std::thread(&BridgeServer::broadcastLoop, this);

    std::cout << "Bridge server started on port " << port << std::endl;
    return true;
}

void BridgeServer::stop() {
    if (!running_) {
        return;
    }

    // Stop broadcast loop
    broadcastRunning_ = false;
    if (broadcastThread_.joinable()) {
        broadcastThread_.join();
    }

    // Stop server
    server_->stop();
    running_ = false;

    std::cout << "Bridge server stopped" << std::endl;
}

void BridgeServer::setKinectDevice(KinectDevice* device) {
    kinectDevice_ = device;

    if (device) {
        // Set up callbacks
        device->setDepthCallback([this](const void* data, uint32_t timestamp) {
            onDepthFrame(data, timestamp);
        });

        device->setVideoCallback([this](const void* data, uint32_t timestamp) {
            onVideoFrame(data, timestamp);
        });

        kinectConnected_ = true;
    } else {
        kinectConnected_ = false;
    }
}

size_t BridgeServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clients_.size();
}

void BridgeServer::onConnection(ix::WebSocket* ws) {
    if (!ws) return;

    std::cout << "Client connected" << std::endl;

    // Add to clients map
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_[ws] = ClientState{};
    }

    // Send hello
    sendHello(ws);
}

void BridgeServer::onMessage(ix::WebSocket* ws, const std::string& message) {
    if (!ws) return;

    try {
        auto msg = json::parse(message);
        std::string type = msg.value("type", "");

        if (type == "subscribe") {
            handleSubscribe(ws, message);
        } else if (type == "unsubscribe") {
            handleUnsubscribe(ws);
        } else {
            sendError(ws, "PROTOCOL_ERROR", "Unknown message type: " + type, true);
        }
    } catch (const json::parse_error& e) {
        sendError(ws, "PROTOCOL_ERROR", "Invalid JSON: " + std::string(e.what()), true);
    }
}

void BridgeServer::onClose(ix::WebSocket* ws) {
    if (!ws) return;

    std::cout << "Client disconnected" << std::endl;

    std::lock_guard<std::mutex> lock(clientsMutex_);
    clients_.erase(ws);
}

void BridgeServer::handleSubscribe(ix::WebSocket* ws, const std::string& message) {
    if (!ws) return;

    try {
        auto msg = json::parse(message);
        auto streams = msg.value("streams", std::vector<std::string>{});

        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(ws);
        if (it != clients_.end()) {
            it->second.subscribedRgb = false;
            it->second.subscribedDepth = false;

            for (const auto& stream : streams) {
                if (stream == "rgb") {
                    it->second.subscribedRgb = true;
                } else if (stream == "depth") {
                    it->second.subscribedDepth = true;
                }
            }

            std::cout << "Client subscribed to: ";
            if (it->second.subscribedRgb) std::cout << "rgb ";
            if (it->second.subscribedDepth) std::cout << "depth ";
            std::cout << std::endl;
        }
    } catch (const json::parse_error& e) {
        sendError(ws, "PROTOCOL_ERROR", "Invalid subscribe message", true);
    }
}

void BridgeServer::handleUnsubscribe(ix::WebSocket* ws) {
    if (!ws) return;

    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(ws);
    if (it != clients_.end()) {
        it->second.subscribedRgb = false;
        it->second.subscribedDepth = false;
    }

    std::cout << "Client unsubscribed" << std::endl;
}

void BridgeServer::sendHello(ix::WebSocket* ws) {
    json hello = {
        {"type", "hello"},
        {"protocol_version", PROTOCOL_VERSION},
        {"server", SERVER_NAME},
        {"capabilities", {
            {"streams", {"rgb", "depth"}},
            {"rgb", {
                {"width", FRAME_WIDTH},
                {"height", FRAME_HEIGHT},
                {"format", "RGB888"},
                {"bytes_per_frame", RGB_FRAME_SIZE}
            }},
            {"depth", {
                {"width", FRAME_WIDTH},
                {"height", FRAME_HEIGHT},
                {"format", "UINT16"},
                {"bits_per_pixel", 16},
                {"bytes_per_frame", DEPTH_FRAME_SIZE},
                {"min_depth_mm", 800},
                {"max_depth_mm", 4000}
            }},
            {"frame_rate_hz", 30}
        }}
    };

    ws->send(hello.dump());
}

void BridgeServer::sendError(ix::WebSocket* ws, const std::string& code,
                              const std::string& message, bool recoverable) {
    json error = {
        {"type", "error"},
        {"code", code},
        {"message", message},
        {"recoverable", recoverable}
    };

    ws->send(error.dump());
}

void BridgeServer::sendStatus(ix::WebSocket* ws) {
    json status = {
        {"type", "status"},
        {"kinect_connected", kinectConnected_ || mockMode_},
        {"frame_id", frameCache_.frameId},
        {"dropped_frames", droppedFrames_.load()},
        {"clients_connected", getClientCount()}
    };

    ws->send(status.dump());
}

void BridgeServer::broadcastLoop() {
    using namespace std::chrono;

    auto nextFrameTime = steady_clock::now();

    while (broadcastRunning_) {
        auto now = steady_clock::now();

        if (now >= nextFrameTime) {
            // Time to send a frame
            std::vector<uint8_t> rgbData;
            std::vector<uint8_t> depthData;
            uint32_t frameId;
            bool hasRgb = false;
            bool hasDepth = false;

            // Get frame data
            {
                std::lock_guard<std::mutex> lock(frameCache_.mutex);

                if (mockMode_) {
                    // Generate mock data
                    frameCache_.frameId++;
                    generateMockRgbFrame(frameCache_.rgbData, frameCache_.frameId);
                    generateMockDepthFrame(frameCache_.depthData, frameCache_.frameId);
                    frameCache_.rgbValid = true;
                    frameCache_.depthValid = true;
                }

                frameId = frameCache_.frameId;

                if (frameCache_.rgbValid) {
                    rgbData = frameCache_.rgbData;
                    hasRgb = true;
                }

                if (frameCache_.depthValid) {
                    depthData = frameCache_.depthData;
                    hasDepth = true;
                }
            }

            // Broadcast to subscribed clients
            if (hasRgb) {
                broadcastFrame(STREAM_TYPE_RGB, rgbData, frameId);
            }
            if (hasDepth) {
                broadcastFrame(STREAM_TYPE_DEPTH, depthData, frameId);
            }

            // Schedule next frame
            nextFrameTime += milliseconds(FRAME_INTERVAL_MS);

            // If we're behind, skip ahead
            if (nextFrameTime < now) {
                auto skipped = duration_cast<milliseconds>(now - nextFrameTime).count() / FRAME_INTERVAL_MS;
                droppedFrames_ += skipped;
                nextFrameTime = now + milliseconds(FRAME_INTERVAL_MS);
            }
        } else {
            // Sleep until next frame
            std::this_thread::sleep_until(nextFrameTime);
        }
    }
}

void BridgeServer::broadcastFrame(uint16_t streamType, const std::vector<uint8_t>& data, uint32_t frameId) {
    // Build binary message: 8-byte header + pixel data
    std::vector<uint8_t> message(8 + data.size());

    // Header (little-endian)
    message[0] = frameId & 0xFF;
    message[1] = (frameId >> 8) & 0xFF;
    message[2] = (frameId >> 16) & 0xFF;
    message[3] = (frameId >> 24) & 0xFF;
    message[4] = streamType & 0xFF;
    message[5] = (streamType >> 8) & 0xFF;
    message[6] = 0;  // Reserved
    message[7] = 0;  // Reserved

    // Pixel data
    std::memcpy(message.data() + 8, data.data(), data.size());

    // Broadcast to subscribed clients
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (auto& [wsPtr, state] : clients_) {
        bool shouldSend = (streamType == STREAM_TYPE_RGB && state.subscribedRgb) ||
                          (streamType == STREAM_TYPE_DEPTH && state.subscribedDepth);

        if (shouldSend) {
            // Note: IXWebSocket needs the raw pointer for sending
            // This is safe because we're under the clients mutex
            wsPtr->sendBinary(std::string(reinterpret_cast<char*>(message.data()), message.size()));
            framesSent_++;
        }
    }
}

void BridgeServer::onDepthFrame(const void* data, uint32_t timestamp) {
    std::lock_guard<std::mutex> lock(frameCache_.mutex);

    // Copy depth data (640x480 uint16_t = 614400 bytes)
    std::memcpy(frameCache_.depthData.data(), data, DEPTH_FRAME_SIZE);
    frameCache_.depthTimestamp = timestamp;
    frameCache_.depthValid = true;
    frameCache_.frameId++;

    // Debug: Log occasionally
    static int depthCount = 0;
    if (++depthCount % 100 == 0) {
        std::cout << "[DEBUG] Depth callback fired " << depthCount << " times" << std::endl;
    }
}

void BridgeServer::onVideoFrame(const void* data, uint32_t timestamp) {
    std::lock_guard<std::mutex> lock(frameCache_.mutex);

    // Copy RGB data (640x480x3 = 921600 bytes)
    std::memcpy(frameCache_.rgbData.data(), data, RGB_FRAME_SIZE);
    frameCache_.rgbTimestamp = timestamp;
    frameCache_.rgbValid = true;

    // Debug: Log occasionally
    static int rgbCount = 0;
    if (++rgbCount % 100 == 0) {
        std::cout << "[DEBUG] RGB callback fired " << rgbCount << " times" << std::endl;
    }
}

void BridgeServer::generateMockRgbFrame(std::vector<uint8_t>& data, uint32_t frameId) {
    // Generate a simple test pattern: moving gradient
    for (uint32_t y = 0; y < FRAME_HEIGHT; y++) {
        for (uint32_t x = 0; x < FRAME_WIDTH; x++) {
            uint32_t idx = (y * FRAME_WIDTH + x) * 3;
            data[idx + 0] = static_cast<uint8_t>((x + frameId * 2) % 256);      // R
            data[idx + 1] = static_cast<uint8_t>((y + frameId) % 256);          // G
            data[idx + 2] = static_cast<uint8_t>((x + y + frameId * 3) % 256);  // B
        }
    }
}

void BridgeServer::generateMockDepthFrame(std::vector<uint8_t>& data, uint32_t frameId) {
    // Generate a simple depth pattern: waves
    auto* depthPtr = reinterpret_cast<uint16_t*>(data.data());

    for (uint32_t y = 0; y < FRAME_HEIGHT; y++) {
        for (uint32_t x = 0; x < FRAME_WIDTH; x++) {
            // Simulate depth: center closer, edges farther
            float dx = (x - FRAME_WIDTH / 2.0f) / (FRAME_WIDTH / 2.0f);
            float dy = (y - FRAME_HEIGHT / 2.0f) / (FRAME_HEIGHT / 2.0f);
            float dist = std::sqrt(dx * dx + dy * dy);

            // Oscillate with frame ID for movement
            float wave = std::sin(dist * 10.0f - frameId * 0.1f) * 0.1f;

            // Depth in mm (800-4000 range)
            uint16_t depth = static_cast<uint16_t>(800 + (dist + wave) * 3200);
            depth = std::min(depth, static_cast<uint16_t>(4000));

            depthPtr[y * FRAME_WIDTH + x] = depth;
        }
    }
}

}  // namespace kinect_xr
