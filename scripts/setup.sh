#!/usr/bin/env bash
#
# setup.sh - Install dependencies and build Kinect XR
#
# Usage: ./scripts/setup.sh
#

set -e  # Exit on error

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
log_header() { echo ""; echo "${BOLD}${BLUE}==> $1${RESET}"; }

# Track if any errors occurred
ERRORS=0

# Function to prompt for installation
prompt_install() {
  local package="$1"
  local description="$2"
  echo ""
  echo "${YELLOW}$package${RESET} is not installed."
  echo "Description: $description"
  read -p "Install $package? (y/n): " -n 1 -r
  echo
  if [[ $REPLY =~ ^[Yy]$ ]]; then
    return 0
  else
    return 1
  fi
}

# Get project root (parent of scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

log_header "Kinect XR Setup"
log_info "Project root: $PROJECT_ROOT"

# 1. Check Xcode Command Line Tools
log_header "Checking Xcode Command Line Tools"
if xcode-select -p &> /dev/null; then
  log_skip "Already installed: $(xcode-select -p)"
else
  log_info "Installing Xcode Command Line Tools..."
  if prompt_install "Xcode Command Line Tools" "Required for C++ compilation"; then
    xcode-select --install
    echo "Please complete the Xcode CLI tools installation dialog, then re-run this script."
    exit 1
  else
    log_error "Xcode Command Line Tools are required. Cannot continue."
    exit 1
  fi
fi

# 2. Check Homebrew
log_header "Checking Homebrew"
if command -v brew &> /dev/null; then
  log_skip "Already installed: $(brew --version | head -n1)"
else
  log_info "Homebrew not found."
  if prompt_install "Homebrew" "Package manager for macOS dependencies"; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    # Source Homebrew for ARM Macs
    if [[ -f /opt/homebrew/bin/brew ]]; then
      eval "$(/opt/homebrew/bin/brew shellenv)"
    fi
    log_success "Homebrew installed"
  else
    log_error "Homebrew is required for dependency installation. Cannot continue."
    exit 1
  fi
fi

# 3. Check CMake
log_header "Checking CMake"
if command -v cmake &> /dev/null; then
  CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
  log_skip "Already installed: CMake $CMAKE_VERSION"
else
  log_info "CMake not found."
  if prompt_install "cmake" "Build system generator (required for compilation)"; then
    brew install cmake
    log_success "CMake installed"
  else
    log_error "CMake is required for building. Cannot continue."
    exit 1
  fi
fi

# 4. Check libfreenect
log_header "Checking libfreenect"
if brew list libfreenect &> /dev/null 2>&1; then
  FREENECT_VERSION=$(brew list --versions libfreenect | cut -d' ' -f2)
  log_skip "Already installed: libfreenect $FREENECT_VERSION"
else
  log_info "libfreenect not found."
  if prompt_install "libfreenect" "Kinect hardware driver (required for device access)"; then
    brew install libfreenect
    log_success "libfreenect installed"
  else
    log_error "libfreenect is required for Kinect support. Cannot continue."
    exit 1
  fi
fi

# 5. Check Bun (for web server)
log_header "Checking Bun"
if command -v bun &> /dev/null; then
  BUN_VERSION=$(bun --version)
  log_skip "Already installed: Bun $BUN_VERSION"
else
  log_info "Bun not found."
  if prompt_install "bun" "JavaScript runtime for web server (optional, but recommended)"; then
    curl -fsSL https://bun.sh/install | bash
    # Source bun for current shell
    if [[ -f "$HOME/.bun/bin/bun" ]]; then
      export PATH="$HOME/.bun/bin:$PATH"
    fi
    log_success "Bun installed"
  else
    log_skip "Skipping Bun (web server will not be available)"
  fi
fi

# 6. Build project
log_header "Building Kinect XR"
cd "$PROJECT_ROOT"

# Create build directory if it doesn't exist
if [[ ! -d "build" ]]; then
  log_info "Creating build directory..."
  mkdir -p build
fi

# Run CMake configure
log_info "Configuring with CMake..."
if cmake -B build -S . -DCMAKE_BUILD_TYPE=Release; then
  log_success "CMake configuration complete"
else
  log_error "CMake configuration failed"
  ERRORS=$((ERRORS + 1))
fi

# Run build
if [[ $ERRORS -eq 0 ]]; then
  log_info "Building binaries..."
  if cmake --build build --config Release -j$(sysctl -n hw.ncpu); then
    log_success "Build complete"
  else
    log_error "Build failed"
    ERRORS=$((ERRORS + 1))
  fi
fi

# 7. Create PID directory for start.sh
log_header "Setting up runtime directories"
mkdir -p "$PROJECT_ROOT/tmp/.pids"
log_success "Created tmp/.pids/"

# Summary
echo ""
log_header "Setup Summary"

if [[ $ERRORS -eq 0 ]]; then
  log_success "Setup complete!"
  echo ""
  echo "Binaries built:"
  if [[ -f "$PROJECT_ROOT/build/bin/kinect-bridge" ]]; then
    log_success "kinect-bridge: $PROJECT_ROOT/build/bin/kinect-bridge"
  fi
  if [[ -f "$PROJECT_ROOT/build/bin/kinect_xr_runtime" ]]; then
    log_success "kinect_xr_runtime: $PROJECT_ROOT/build/bin/kinect_xr_runtime"
  fi

  echo ""
  echo "Next steps:"
  echo "  ${BOLD}./scripts/start.sh${RESET}     # Start bridge and web server"
  echo "  ${BOLD}./scripts/stop.sh${RESET}      # Stop all services"
  echo ""
  echo "Optional: Configure passwordless sudo for hardware tests:"
  echo "  ${BOLD}./scripts/setup-test-permissions.sh${RESET}"
else
  log_error "Setup completed with $ERRORS error(s)"
  exit 1
fi
