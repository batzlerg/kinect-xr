#!/bin/bash
# Start Kinect XR Bridge and Web Server
# Usage: ./start.sh
#
# Prerequisites: Run scripts/setup-test-permissions.sh once to configure sudoers

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
WEB_DIR="$SCRIPT_DIR/web"

# Colors
RED='\033[1;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Kinect XR Bridge Startup${NC}"
echo "========================"
echo ""

# Check if bridge binary exists
if [ ! -f "$BUILD_DIR/bin/kinect-bridge" ]; then
    echo "ERROR: Bridge binary not found. Run: cmake --build build"
    exit 1
fi

# Check if web server exists
if [ ! -f "$WEB_DIR/serve.js" ]; then
    echo "ERROR: Web server not found at $WEB_DIR/serve.js"
    exit 1
fi

# Clean up any existing processes on our ports
echo -e "${YELLOW}Cleaning up old processes...${NC}"
sudo lsof -ti:8765 | xargs sudo kill -9 2>/dev/null || true
lsof -ti:3000 | xargs kill -9 2>/dev/null || true
sleep 1

# Cleanup function
cleanup() {
    echo ""
    echo "Shutting down..."
    kill $BRIDGE_PID 2>/dev/null || true
    kill $WEB_PID 2>/dev/null || true
    exit 0
}
trap cleanup SIGINT SIGTERM

# Start bridge server (requires sudo for USB access)
echo -e "${GREEN}Starting bridge server...${NC}"
sudo "$BUILD_DIR/bin/kinect-bridge" &
BRIDGE_PID=$!

# Wait a moment for bridge to start and check if it's still running
sleep 2

if ! kill -0 $BRIDGE_PID 2>/dev/null; then
    # Bridge exited - get exit code
    wait $BRIDGE_PID
    EXIT_CODE=$?

    echo ""
    echo -e "${RED}========================================"
    echo -e "  BRIDGE SERVER FAILED TO START"
    echo -e "========================================${NC}"
    echo ""

    case $EXIT_CODE in
        2)
            echo -e "${YELLOW}No Kinect device was detected.${NC}"
            echo ""
            echo "  Try these fixes:"
            echo "    1. Unplug and replug the Kinect USB cable"
            echo "    2. Check that the Kinect power supply is connected"
            echo "    3. Wait 5 seconds after plugging in, then try again"
            ;;
        3)
            echo -e "${YELLOW}Kinect was detected but failed to initialize.${NC}"
            echo ""
            echo "  This usually means the USB connection is stuck."
            echo ""
            echo "  Fix: Unplug and replug the Kinect USB cable"
            ;;
        *)
            echo -e "${YELLOW}Bridge server exited with code $EXIT_CODE${NC}"
            ;;
    esac

    echo ""
    echo "  Alternative: Run with mock data for testing:"
    echo "    sudo $BUILD_DIR/bin/kinect-bridge --mock"
    echo ""
    exit 1
fi

# Start web server
echo -e "${GREEN}Starting web server...${NC}"
cd "$WEB_DIR"
bun serve.js &
WEB_PID=$!
cd "$SCRIPT_DIR"

echo ""
echo -e "${GREEN}Both servers running!${NC}"
echo ""
echo "  Dashboard: http://localhost:3000/examples/rgb-depth/"
echo "  WebSocket: ws://localhost:8765/kinect"
echo ""
echo "Press Ctrl+C to stop"
echo ""

# Wait for either process to exit
wait $BRIDGE_PID $WEB_PID
WAIT_EXIT=$?

# If bridge exited unexpectedly, check its exit code
if ! kill -0 $BRIDGE_PID 2>/dev/null; then
    wait $BRIDGE_PID 2>/dev/null
    BRIDGE_EXIT=$?
    if [ $BRIDGE_EXIT -ne 0 ]; then
        echo ""
        echo -e "${RED}Bridge server exited with error (code $BRIDGE_EXIT)${NC}"
        if [ $BRIDGE_EXIT -eq 2 ] || [ $BRIDGE_EXIT -eq 3 ]; then
            echo -e "${YELLOW}Kinect USB may have disconnected. Replug and restart.${NC}"
        fi
    fi
fi

# Clean up web server if still running
kill $WEB_PID 2>/dev/null || true
