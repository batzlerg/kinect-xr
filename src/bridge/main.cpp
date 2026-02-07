/**
 * @file main.cpp
 * @brief Kinect XR WebSocket Bridge Server
 *
 * Standalone executable that streams Kinect RGB + depth to browser clients.
 *
 * Usage:
 *   kinect-bridge              # Start with Kinect (requires sudo on macOS)
 *   kinect-bridge --mock       # Start with mock data (no Kinect required)
 *   kinect-bridge --port 9000  # Use custom port
 */

#include "kinect_xr/bridge_server.h"
#include "kinect_xr/device.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {
kinect_xr::BridgeServer* g_server = nullptr;

// ANSI color codes
const char* RED = "\033[1;31m";
const char* YELLOW = "\033[1;33m";
const char* GREEN = "\033[0;32m";
const char* RESET = "\033[0m";

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

void printUsage(const char* progName) {
    std::cout << "Kinect XR WebSocket Bridge Server\n"
              << "\n"
              << "Usage: " << progName << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  --mock       Use mock data (no Kinect required)\n"
              << "  --port PORT  Listen on PORT (default: 8765)\n"
              << "  --help       Show this help\n"
              << "\n"
              << "Note: Kinect mode requires elevated privileges on macOS.\n"
              << "      Run with sudo or use --mock for testing.\n";
}

void printDeviceError(const char* message, const char* suggestion = nullptr) {
    std::cerr << "\n";
    std::cerr << RED << "========================================" << RESET << "\n";
    std::cerr << RED << "  KINECT DEVICE ERROR" << RESET << "\n";
    std::cerr << RED << "========================================" << RESET << "\n";
    std::cerr << "\n";
    std::cerr << RED << "  " << message << RESET << "\n";
    std::cerr << "\n";
    if (suggestion) {
        std::cerr << YELLOW << "  FIX: " << suggestion << RESET << "\n";
        std::cerr << "\n";
    }
    std::cerr << "  Alternatives:\n";
    std::cerr << "    - Run with --mock for testing without hardware\n";
    std::cerr << "    - Check USB connection and power\n";
    std::cerr << "\n";
    std::cerr << RED << "========================================" << RESET << "\n";
    std::cerr << "\n";
}
}  // namespace

int main(int argc, char* argv[]) {
    int port = 8765;
    bool mockMode = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--mock") == 0) {
            mockMode = true;
        } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    std::cout << "Kinect XR WebSocket Bridge Server" << std::endl;
    std::cout << "=================================" << std::endl;

    // Create bridge server
    kinect_xr::BridgeServer server;
    g_server = &server;

    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Initialize Kinect if not in mock mode
    std::unique_ptr<kinect_xr::KinectDevice> kinect;

    if (mockMode) {
        std::cout << "Mode: Mock data (no Kinect)" << std::endl;
        server.setMockMode(true);
    } else {
        std::cout << "Mode: Kinect hardware" << std::endl;

        // Check for Kinect
        int deviceCount = kinect_xr::KinectDevice::getDeviceCount();
        if (deviceCount == 0) {
            printDeviceError(
                "No Kinect device found.",
                "Unplug and replug the Kinect USB cable, then try again."
            );
            return 2;  // Exit code 2 = no device found
        }

        std::cout << "Found " << deviceCount << " Kinect device(s)" << std::endl;

        // Initialize Kinect
        kinect = std::make_unique<kinect_xr::KinectDevice>();
        kinect_xr::DeviceConfig config;
        config.enableRGB = true;
        config.enableDepth = true;
        config.enableMotor = true;  // Enable motor for tilt control

        auto error = kinect->initialize(config);
        if (error != kinect_xr::DeviceError::None) {
            printDeviceError(
                ("Kinect initialization failed: " + kinect_xr::errorToString(error)).c_str(),
                "Unplug and replug the Kinect USB cable, then try again."
            );
            return 3;  // Exit code 3 = init failed
        }

        std::cout << GREEN << "Kinect initialized successfully" << RESET << std::endl;

        // Connect to bridge server (streams will start on-demand when clients connect)
        server.setKinectDevice(kinect.get());

        std::cout << "Kinect ready (streams will start when clients connect)" << std::endl;
    }

    // Start server
    std::cout << "Starting WebSocket server on port " << port << "..." << std::endl;

    if (!server.start(port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "\nBridge running. Connect browsers to ws://localhost:" << port << "/kinect" << std::endl;
    std::cout << "Press Ctrl+C to stop.\n" << std::endl;

    // Main loop - just wait for signal
    // (Stats are now printed automatically by broadcast loop every 10s)
    while (server.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Cleanup (streams are stopped automatically when server stops)
    std::cout << "Stopping server..." << std::endl;

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
