#!/usr/bin/env bash
# Install Kinect XR runtime system-wide on macOS
# Usage: ./scripts/install-runtime.sh

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Installation paths
LIB_SOURCE="$REPO_DIR/build/lib/libkinect_xr_runtime_lib.dylib"
LIB_DEST="/usr/local/lib/libkinect_xr_runtime.dylib"
MANIFEST_DIR="/usr/local/share/kinect-xr"
MANIFEST_DEST="$MANIFEST_DIR/kinect_xr.json"
OPENXR_DIR="/usr/local/share/openxr/1"
ACTIVE_RUNTIME="$OPENXR_DIR/active_runtime.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "Kinect XR Runtime Installation"
echo "=============================="
echo ""

# Check if library exists
if [ ! -f "$LIB_SOURCE" ]; then
  echo -e "${RED}Error: Runtime library not found at:${NC}"
  echo "  $LIB_SOURCE"
  echo ""
  echo "Please build the project first:"
  echo "  cmake -B build -S ."
  echo "  cmake --build build"
  exit 1
fi

# Check if running as root (we'll use sudo for individual commands)
if [ "$EUID" -eq 0 ]; then
  echo -e "${YELLOW}Warning: Don't run this script with sudo directly.${NC}"
  echo "The script will prompt for sudo when needed."
  exit 1
fi

echo "Installation plan:"
echo "  Library: $LIB_DEST"
echo "  Manifest: $MANIFEST_DEST"
echo "  Active runtime: $ACTIVE_RUNTIME"
echo ""

# Backup existing active runtime symlink if it exists and points elsewhere
if [ -L "$ACTIVE_RUNTIME" ]; then
  CURRENT_TARGET=$(readlink "$ACTIVE_RUNTIME")
  if [ "$CURRENT_TARGET" != "$MANIFEST_DEST" ]; then
    BACKUP_PATH="${ACTIVE_RUNTIME}.backup.$(date +%s)"
    echo -e "${YELLOW}Backing up existing runtime symlink to:${NC}"
    echo "  $BACKUP_PATH"
    sudo cp -P "$ACTIVE_RUNTIME" "$BACKUP_PATH"
  fi
elif [ -f "$ACTIVE_RUNTIME" ]; then
  echo -e "${YELLOW}Warning: $ACTIVE_RUNTIME exists but is not a symlink${NC}"
  BACKUP_PATH="${ACTIVE_RUNTIME}.backup.$(date +%s)"
  echo "Backing up to: $BACKUP_PATH"
  sudo mv "$ACTIVE_RUNTIME" "$BACKUP_PATH"
fi

echo "Installing runtime library..."
sudo cp "$LIB_SOURCE" "$LIB_DEST"
echo -e "${GREEN}✓${NC} Library installed to $LIB_DEST"

echo "Creating manifest directory..."
sudo mkdir -p "$MANIFEST_DIR"

echo "Generating manifest..."
cat > /tmp/kinect_xr_manifest.json <<EOF
{
    "file_format_version": "1.0.0",
    "runtime": {
        "name": "Kinect XR Runtime",
        "library_path": "$LIB_DEST"
    }
}
EOF

sudo mv /tmp/kinect_xr_manifest.json "$MANIFEST_DEST"
echo -e "${GREEN}✓${NC} Manifest installed to $MANIFEST_DEST"

echo "Creating OpenXR runtime directory..."
sudo mkdir -p "$OPENXR_DIR"

echo "Creating active runtime symlink..."
sudo ln -sf "$MANIFEST_DEST" "$ACTIVE_RUNTIME"
echo -e "${GREEN}✓${NC} Active runtime symlink created at $ACTIVE_RUNTIME"

echo ""
echo -e "${GREEN}Installation complete!${NC}"
echo ""
echo "The Kinect XR runtime is now the system-wide default."
echo "All OpenXR applications will use it automatically."
echo ""
echo "To uninstall, run:"
echo "  ./scripts/uninstall-runtime.sh"
