#!/usr/bin/env bash
#
# start.sh - Start Kinect bridge and web server
#
# Usage: ./scripts/start.sh [--mock]
#
# Options:
#   --mock    Run bridge in mock mode (no hardware required)
#

set -e

# Color output helpers
if command -v tput &> /dev/null && [[ -t 1 ]]; then
  RED=$(tput setaf 1)
  GREEN=$(tput setaf 2)
  YELLOW=$(tput setaf 3)
  BLUE=$(tput setaf 4)
  BOLD=$(tput bold)
  RESET=$(tput sgr0)
else
  RED=""
  GREEN=""
  YELLOW=""
  BLUE=""
  BOLD=""
  RESET=""
fi

log_success() { echo "${GREEN}✓${RESET} $1"; }
log_info() { echo "${BLUE}ℹ${RESET} $1"; }
log_error() { echo "${RED}✗${RESET} $1" >&2; }
log_header() { echo ""; echo "${BOLD}${BLUE}==> $1${RESET}"; }

# Get project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Parse arguments
MOCK_MODE=false
if [[ "$1" == "--mock" ]]; then
  MOCK_MODE=true
fi

# PID directory
PID_DIR="$PROJECT_ROOT/tmp/.pids"
BRIDGE_PID_FILE="$PID_DIR/kinect-bridge.pid"
WEB_PID_FILE="$PID_DIR/web-server.pid"

# Ensure PID directory exists
mkdir -p "$PID_DIR"

log_header "Starting Kinect XR"

# 1. Check if binary exists
BRIDGE_BIN="$PROJECT_ROOT/build/bin/kinect-bridge"
if [[ ! -f "$BRIDGE_BIN" ]]; then
  log_error "kinect-bridge binary not found at: $BRIDGE_BIN"
  echo ""
  echo "Please run setup first:"
  echo "  ${BOLD}./scripts/setup.sh${RESET}"
  exit 1
fi

# 2. Check if already running
if [[ -f "$BRIDGE_PID_FILE" ]]; then
  OLD_PID=$(cat "$BRIDGE_PID_FILE")
  if ps -p "$OLD_PID" > /dev/null 2>&1; then
    log_error "kinect-bridge is already running (PID $OLD_PID)"
    echo "Stop it first with: ${BOLD}./scripts/stop.sh${RESET}"
    exit 1
  else
    # Stale PID file
    rm "$BRIDGE_PID_FILE"
  fi
fi

if [[ -f "$WEB_PID_FILE" ]]; then
  OLD_PID=$(cat "$WEB_PID_FILE")
  if ps -p "$OLD_PID" > /dev/null 2>&1; then
    log_error "Web server is already running (PID $OLD_PID)"
    echo "Stop it first with: ${BOLD}./scripts/stop.sh${RESET}"
    exit 1
  else
    rm "$WEB_PID_FILE"
  fi
fi

# 3. Determine if sudo is needed
NEED_SUDO=false
if [[ "$MOCK_MODE" == false ]]; then
  # Check if we're already root
  if [[ $EUID -eq 0 ]]; then
    NEED_SUDO=false
  else
    # Check if passwordless sudo is configured
    if sudo -n true 2>/dev/null; then
      NEED_SUDO=true
    else
      # Need to prompt for sudo
      log_info "Hardware access requires elevated privileges"
      echo "You may be prompted for your password."
      if sudo -v; then
        NEED_SUDO=true
      else
        log_error "Failed to obtain sudo privileges"
        exit 1
      fi
    fi
  fi
fi

# 4. Start kinect-bridge
log_header "Starting Kinect Bridge"

if [[ "$MOCK_MODE" == true ]]; then
  log_info "Running in MOCK mode (no hardware required)"
  BRIDGE_ARGS="--mock"
else
  log_info "Running in HARDWARE mode"
  BRIDGE_ARGS=""
fi

# Start bridge in background
if [[ "$NEED_SUDO" == true ]]; then
  log_info "Starting with sudo (hardware access)..."
  sudo "$BRIDGE_BIN" $BRIDGE_ARGS > "$PROJECT_ROOT/tmp/bridge.log" 2>&1 &
  BRIDGE_PID=$!
else
  "$BRIDGE_BIN" $BRIDGE_ARGS > "$PROJECT_ROOT/tmp/bridge.log" 2>&1 &
  BRIDGE_PID=$!
fi

# Save PID
echo "$BRIDGE_PID" > "$BRIDGE_PID_FILE"

# Wait for bridge to initialize
log_info "Waiting for bridge to initialize..."
sleep 2

# Check if bridge is still running
if ! ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
  log_error "Bridge failed to start"
  echo "Check logs: ${BOLD}$PROJECT_ROOT/tmp/bridge.log${RESET}"
  rm "$BRIDGE_PID_FILE"
  exit 1
fi

log_success "Bridge started (PID $BRIDGE_PID)"

# 5. Check if Bun is available for web server
if ! command -v bun &> /dev/null; then
  log_error "Bun not found. Cannot start web server."
  echo "Install Bun with: ${BOLD}curl -fsSL https://bun.sh/install | bash${RESET}"
  echo "Or run setup: ${BOLD}./scripts/setup.sh${RESET}"
  echo ""
  echo "Bridge is running, but web interface is not available."
  exit 1
fi

# 6. Start web server
log_header "Starting Web Server"
cd "$PROJECT_ROOT/web"

bun run serve.js > "$PROJECT_ROOT/tmp/web.log" 2>&1 &
WEB_PID=$!
echo "$WEB_PID" > "$WEB_PID_FILE"

# Wait briefly for web server
sleep 1

if ! ps -p "$WEB_PID" > /dev/null 2>&1; then
  log_error "Web server failed to start"
  echo "Check logs: ${BOLD}$PROJECT_ROOT/tmp/web.log${RESET}"
  rm "$WEB_PID_FILE"
  exit 1
fi

log_success "Web server started (PID $WEB_PID)"

# 7. Open browser
log_header "Opening Browser"
EXAMPLE_URL="http://localhost:3000/examples/threejs/"
if command -v open &> /dev/null; then
  open "$EXAMPLE_URL"
  log_success "Browser opened to $EXAMPLE_URL"
else
  log_info "Open manually: $EXAMPLE_URL"
fi

# 8. Print status
echo ""
log_header "Status"
echo "${BOLD}Kinect Bridge:${RESET}"
echo "  PID:  $BRIDGE_PID"
echo "  Log:  $PROJECT_ROOT/tmp/bridge.log"
echo "  URL:  ws://localhost:9001"
echo ""
echo "${BOLD}Web Server:${RESET}"
echo "  PID:  $WEB_PID"
echo "  Log:  $PROJECT_ROOT/tmp/web.log"
echo "  URL:  http://localhost:3000"
echo ""
echo "${BOLD}Examples:${RESET}"
echo "  Three.js:  $EXAMPLE_URL"
echo ""
echo "To stop services: ${BOLD}./scripts/stop.sh${RESET}"
echo "Press ${BOLD}Ctrl+C${RESET} to stop all services now"

# Trap Ctrl+C to kill both processes
cleanup() {
  echo ""
  log_info "Shutting down..."

  if [[ -f "$BRIDGE_PID_FILE" ]]; then
    BRIDGE_PID=$(cat "$BRIDGE_PID_FILE")
    if ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
      if [[ "$NEED_SUDO" == true ]]; then
        sudo kill "$BRIDGE_PID" 2>/dev/null || true
      else
        kill "$BRIDGE_PID" 2>/dev/null || true
      fi
      log_success "Bridge stopped"
    fi
    rm "$BRIDGE_PID_FILE"
  fi

  if [[ -f "$WEB_PID_FILE" ]]; then
    WEB_PID=$(cat "$WEB_PID_FILE")
    if ps -p "$WEB_PID" > /dev/null 2>&1; then
      kill "$WEB_PID" 2>/dev/null || true
      log_success "Web server stopped"
    fi
    rm "$WEB_PID_FILE"
  fi

  exit 0
}

trap cleanup SIGINT SIGTERM

# Wait indefinitely (keep script running to handle Ctrl+C)
while true; do
  sleep 1
  # Check if processes are still alive
  if [[ -f "$BRIDGE_PID_FILE" ]]; then
    BRIDGE_PID=$(cat "$BRIDGE_PID_FILE")
    if ! ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
      log_error "Bridge process died unexpectedly"
      echo "Check logs: ${BOLD}$PROJECT_ROOT/tmp/bridge.log${RESET}"
      cleanup
    fi
  fi

  if [[ -f "$WEB_PID_FILE" ]]; then
    WEB_PID=$(cat "$WEB_PID_FILE")
    if ! ps -p "$WEB_PID" > /dev/null 2>&1; then
      log_error "Web server died unexpectedly"
      echo "Check logs: ${BOLD}$PROJECT_ROOT/tmp/web.log${RESET}"
      cleanup
    fi
  fi
done
