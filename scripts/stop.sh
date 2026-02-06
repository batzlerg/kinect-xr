#!/usr/bin/env bash
#
# stop.sh - Stop Kinect bridge and web server
#
# Usage: ./scripts/stop.sh
#

# Don't use set -e here since we handle errors explicitly
# and need to continue even if some kill commands fail

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
log_skip() { echo "${YELLOW}⊘${RESET} $1"; }
log_info() { echo "${BLUE}ℹ${RESET} $1"; }
log_error() { echo "${RED}✗${RESET} $1" >&2; }
log_header() { echo "${BOLD}${BLUE}==> $1${RESET}"; }

# Get project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# PID directory
PID_DIR="$PROJECT_ROOT/tmp/.pids"
BRIDGE_PID_FILE="$PID_DIR/kinect-bridge.pid"
WEB_PID_FILE="$PID_DIR/web-server.pid"

log_header "Stopping Kinect XR"

STOPPED_COUNT=0
NOTHING_RUNNING=true

# 1. Stop kinect-bridge
if [[ -f "$BRIDGE_PID_FILE" ]]; then
  BRIDGE_PID=$(cat "$BRIDGE_PID_FILE")
  if ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
    NOTHING_RUNNING=false
    log_info "Stopping kinect-bridge (PID $BRIDGE_PID)..."

    # Try normal kill first
    if kill "$BRIDGE_PID" 2>/dev/null; then
      # Wait up to 5 seconds for graceful shutdown
      for i in {1..5}; do
        if ! ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
          break
        fi
        sleep 1
      done

      # If still running, try with sudo
      if ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
        log_info "Trying with sudo..."
        sudo kill "$BRIDGE_PID" 2>/dev/null || true
        sleep 1
      fi

      # Force kill if still alive
      if ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
        log_info "Force killing..."
        sudo kill -9 "$BRIDGE_PID" 2>/dev/null || kill -9 "$BRIDGE_PID" 2>/dev/null || true
      fi

      if ! ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
        log_success "Bridge stopped"
        STOPPED_COUNT=$((STOPPED_COUNT + 1))
      else
        log_error "Failed to stop bridge"
      fi
    else
      # Process might be running as root, try sudo
      if sudo kill "$BRIDGE_PID" 2>/dev/null; then
        sleep 1
        if ! ps -p "$BRIDGE_PID" > /dev/null 2>&1; then
          log_success "Bridge stopped"
          STOPPED_COUNT=$((STOPPED_COUNT + 1))
        fi
      else
        log_error "Failed to stop bridge"
      fi
    fi
  else
    log_skip "Bridge not running (stale PID file)"
  fi
  rm -f "$BRIDGE_PID_FILE" 2>/dev/null || true
else
  # Try to find by process name as fallback
  if pgrep -x "kinect-bridge" > /dev/null; then
    NOTHING_RUNNING=false
    log_info "Found kinect-bridge by name (no PID file)..."
    pkill -x "kinect-bridge" 2>/dev/null || sudo pkill -x "kinect-bridge" 2>/dev/null || true
    sleep 1
    if ! pgrep -x "kinect-bridge" > /dev/null; then
      log_success "Bridge stopped"
      STOPPED_COUNT=$((STOPPED_COUNT + 1))
    fi
  fi
fi

# 2. Stop web server
if [[ -f "$WEB_PID_FILE" ]]; then
  WEB_PID=$(cat "$WEB_PID_FILE")
  if ps -p "$WEB_PID" > /dev/null 2>&1; then
    NOTHING_RUNNING=false
    log_info "Stopping web server (PID $WEB_PID)..."

    kill "$WEB_PID" 2>/dev/null || true
    # Wait briefly for shutdown
    sleep 1

    if ! ps -p "$WEB_PID" > /dev/null 2>&1; then
      log_success "Web server stopped"
      STOPPED_COUNT=$((STOPPED_COUNT + 1))
    else
      # Force kill
      kill -9 "$WEB_PID" 2>/dev/null || true
      log_success "Web server stopped (forced)"
      STOPPED_COUNT=$((STOPPED_COUNT + 1))
    fi
  else
    log_skip "Web server not running (stale PID file)"
  fi
  rm -f "$WEB_PID_FILE" 2>/dev/null || true
else
  # Try to find bun serve.js as fallback
  BUN_PID=$(pgrep -f "bun.*serve.js" 2>/dev/null || true)
  if [[ -n "$BUN_PID" ]]; then
    NOTHING_RUNNING=false
    log_info "Found web server by name (no PID file)..."
    kill "$BUN_PID" 2>/dev/null || true
    sleep 1
    if ! ps -p "$BUN_PID" > /dev/null 2>&1; then
      log_success "Web server stopped"
      STOPPED_COUNT=$((STOPPED_COUNT + 1))
    fi
  fi
fi

# Summary
echo ""
if [[ "$NOTHING_RUNNING" == true ]]; then
  log_skip "No services were running"
elif [[ $STOPPED_COUNT -gt 0 ]]; then
  log_success "Stopped $STOPPED_COUNT service(s)"
else
  log_error "Failed to stop services"
  exit 1
fi
