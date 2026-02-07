/**
 * @file bridge_server.h
 * @brief WebSocket bridge server for streaming Kinect data to browsers
 *
 * Implements the protocol defined in specs/008-WebSocketBridgeProtocol.md
 */

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ix {
class WebSocketServer;
class WebSocket;
}  // namespace ix

namespace kinect_xr {

class KinectDevice;
struct MotorStatus;

// Stream types (matches protocol spec)
constexpr uint16_t STREAM_TYPE_RGB = 0x0001;
constexpr uint16_t STREAM_TYPE_DEPTH = 0x0002;

// Frame dimensions
constexpr uint32_t FRAME_WIDTH = 640;
constexpr uint32_t FRAME_HEIGHT = 480;
constexpr uint32_t RGB_FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3;    // 921600
constexpr uint32_t DEPTH_FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 2;  // 614400

/**
 * @brief Client subscription state
 */
struct ClientState {
    bool subscribedRgb = false;
    bool subscribedDepth = false;
};

/**
 * @brief Thread-safe frame cache for latest Kinect data
 */
struct BridgeFrameCache {
    std::mutex mutex;

    std::vector<uint8_t> rgbData;
    uint32_t rgbTimestamp = 0;
    bool rgbValid = false;

    std::vector<uint8_t> depthData;  // Stored as raw bytes (uint16 LE)
    uint32_t depthTimestamp = 0;
    bool depthValid = false;

    uint32_t frameId = 0;

    BridgeFrameCache()
        : rgbData(RGB_FRAME_SIZE), depthData(DEPTH_FRAME_SIZE) {}
};

/**
 * @brief WebSocket bridge server for Kinect → Browser streaming
 *
 * Usage:
 *   BridgeServer server;
 *   server.start(8765);
 *   // ... server runs in background ...
 *   server.stop();
 */
class BridgeServer {
public:
    BridgeServer();
    ~BridgeServer();

    // Non-copyable
    BridgeServer(const BridgeServer&) = delete;
    BridgeServer& operator=(const BridgeServer&) = delete;

    /**
     * @brief Start the bridge server
     * @param port Port to listen on (default: 8765)
     * @return true if started successfully
     */
    bool start(int port = 8765);

    /**
     * @brief Stop the bridge server
     */
    void stop();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running_; }

    /**
     * @brief Set the Kinect device to stream from
     * @param device Pointer to initialized KinectDevice (ownership not transferred)
     */
    void setKinectDevice(KinectDevice* device);

    /**
     * @brief Get number of connected clients
     */
    size_t getClientCount() const;

    /**
     * @brief Enable/disable mock mode (sends test patterns instead of Kinect data)
     */
    void setMockMode(bool enabled) { mockMode_ = enabled; }

    /**
     * @brief Get statistics
     */
    uint32_t getFramesSent() const { return framesSent_; }
    uint32_t getDroppedFrames() const { return droppedFrames_; }

private:
    // WebSocket event handlers
    void onConnection(ix::WebSocket* ws);
    void onMessage(ix::WebSocket* ws, const std::string& message);
    void onClose(ix::WebSocket* ws);

    // Message handlers
    void handleSubscribe(ix::WebSocket* ws, const std::string& message);
    void handleUnsubscribe(ix::WebSocket* ws);
    void handleMotorSetTilt(ix::WebSocket* ws, const std::string& message);
    void handleMotorSetLed(ix::WebSocket* ws, const std::string& message);
    void handleMotorReset(ix::WebSocket* ws);
    void handleMotorGetStatus(ix::WebSocket* ws);

    // Send helpers
    void sendHello(ix::WebSocket* ws);
    void sendError(ix::WebSocket* ws, const std::string& code,
                   const std::string& message, bool recoverable);
    void sendStatus(ix::WebSocket* ws);
    void sendMotorStatus(ix::WebSocket* ws, const MotorStatus& status);
    void sendMotorError(ix::WebSocket* ws, const std::string& code, const std::string& message);

    // Frame broadcasting
    void broadcastLoop();
    void broadcastFrame(uint16_t streamType, const std::vector<uint8_t>& data, uint32_t frameId);

    // Kinect callbacks
    void onDepthFrame(const void* data, uint32_t timestamp);
    void onVideoFrame(const void* data, uint32_t timestamp);

    // Mock data generation
    void generateMockRgbFrame(std::vector<uint8_t>& data, uint32_t frameId);
    void generateMockDepthFrame(std::vector<uint8_t>& data, uint32_t frameId);

    // Server state
    std::unique_ptr<ix::WebSocketServer> server_;
    std::atomic<bool> running_{false};
    int port_ = 8765;

    // Client management
    mutable std::mutex clientsMutex_;
    std::unordered_map<ix::WebSocket*, ClientState> clients_;

    // Frame cache
    BridgeFrameCache frameCache_;

    // Broadcast thread
    std::thread broadcastThread_;
    std::atomic<bool> broadcastRunning_{false};

    // Kinect device
    KinectDevice* kinectDevice_ = nullptr;
    bool kinectConnected_ = false;

    // Mode
    bool mockMode_ = false;

    // Statistics
    std::atomic<uint32_t> framesSent_{0};
    std::atomic<uint32_t> droppedFrames_{0};

    // FPS tracking
    std::atomic<uint32_t> rgbFrameCount_{0};
    std::atomic<uint32_t> depthFrameCount_{0};
    std::chrono::steady_clock::time_point lastStatsTime_;
    std::mutex statsMutex_;

    // Motor control rate limiting (500ms minimum interval)
    std::chrono::steady_clock::time_point lastMotorCommand_;
    std::mutex motorMutex_;

    // Motor status polling
    bool motorMoving_ = false;
    std::chrono::steady_clock::time_point lastMotorStatusCheck_;
    void broadcastMotorStatus(const MotorStatus& status);
};

}  // namespace kinect_xr
