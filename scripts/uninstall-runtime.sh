#!/usr/bin/env bash
# Uninstall Kinect XR runtime from macOS
# Usage: ./scripts/uninstall-runtime.sh

set -e

# Installation paths
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

echo "Kinect XR Runtime Uninstallation"
echo "================================"
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
  echo -e "${YELLOW}Warning: Don't run this script with sudo directly.${NC}"
  echo "The script will prompt for sudo when needed."
  exit 1
fi

# Check what will be removed
FOUND_FILES=0

if [ -L "$ACTIVE_RUNTIME" ]; then
  CURRENT_TARGET=$(readlink "$ACTIVE_RUNTIME")
  if [ "$CURRENT_TARGET" = "$MANIFEST_DEST" ]; then
    echo "Will remove: $ACTIVE_RUNTIME (symlink to Kinect XR)"
    FOUND_FILES=$((FOUND_FILES + 1))
  else
    echo -e "${YELLOW}Note: Active runtime symlink points to a different runtime:${NC}"
    echo "  $CURRENT_TARGET"
    echo "  (will not be removed)"
  fi
elif [ -f "$ACTIVE_RUNTIME" ]; then
  echo -e "${YELLOW}Warning: $ACTIVE_RUNTIME exists but is not a symlink${NC}"
  echo "  (will not be removed)"
fi

if [ -f "$MANIFEST_DEST" ]; then
  echo "Will remove: $MANIFEST_DEST"
  FOUND_FILES=$((FOUND_FILES + 1))
fi

if [ -f "$LIB_DEST" ]; then
  echo "Will remove: $LIB_DEST"
  FOUND_FILES=$((FOUND_FILES + 1))
fi

if [ $FOUND_FILES -eq 0 ]; then
  echo "No Kinect XR runtime installation found."
  echo "Nothing to uninstall."
  exit 0
fi

echo ""
read -p "Proceed with uninstallation? [y/N] " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
  echo "Uninstallation cancelled."
  exit 0
fi

echo ""
echo "Uninstalling..."

# Remove active runtime symlink if it points to our manifest
if [ -L "$ACTIVE_RUNTIME" ]; then
  CURRENT_TARGET=$(readlink "$ACTIVE_RUNTIME")
  if [ "$CURRENT_TARGET" = "$MANIFEST_DEST" ]; then
    echo "Removing active runtime symlink..."
    sudo rm "$ACTIVE_RUNTIME"
    echo -e "${GREEN}✓${NC} Removed $ACTIVE_RUNTIME"

    # Check for backup and offer to restore
    BACKUP_FILES=$(find "$(dirname "$ACTIVE_RUNTIME")" -name "$(basename "$ACTIVE_RUNTIME").backup.*" 2>/dev/null | sort -r || true)
    if [ -n "$BACKUP_FILES" ]; then
      LATEST_BACKUP=$(echo "$BACKUP_FILES" | head -n 1)
      echo ""
      echo -e "${YELLOW}Found backup runtime:${NC}"
      echo "  $LATEST_BACKUP"
      read -p "Restore this backup? [y/N] " -n 1 -r
      echo ""
      if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo cp -P "$LATEST_BACKUP" "$ACTIVE_RUNTIME"
        echo -e "${GREEN}✓${NC} Restored backup"
      fi
    fi
  fi
fi

# Remove manifest
if [ -f "$MANIFEST_DEST" ]; then
  echo "Removing manifest..."
  sudo rm "$MANIFEST_DEST"
  echo -e "${GREEN}✓${NC} Removed $MANIFEST_DEST"

  # Remove manifest directory if empty
  if [ -d "$MANIFEST_DIR" ] && [ -z "$(ls -A "$MANIFEST_DIR")" ]; then
    echo "Removing empty manifest directory..."
    sudo rmdir "$MANIFEST_DIR"
    echo -e "${GREEN}✓${NC} Removed $MANIFEST_DIR"
  fi
fi

# Remove library
if [ -f "$LIB_DEST" ]; then
  echo "Removing runtime library..."
  sudo rm "$LIB_DEST"
  echo -e "${GREEN}✓${NC} Removed $LIB_DEST"
fi

# Clean up OpenXR directory if empty (but be very careful)
if [ -d "$OPENXR_DIR" ] && [ -z "$(ls -A "$OPENXR_DIR")" ]; then
  echo "Removing empty OpenXR directory..."
  sudo rmdir "$OPENXR_DIR"
  echo -e "${GREEN}✓${NC} Removed $OPENXR_DIR"

  # Remove parent directory if also empty
  PARENT_DIR="$(dirname "$OPENXR_DIR")"
  if [ -d "$PARENT_DIR" ] && [ -z "$(ls -A "$PARENT_DIR")" ]; then
    sudo rmdir "$PARENT_DIR"
    echo -e "${GREEN}✓${NC} Removed $PARENT_DIR"
  fi
fi

echo ""
echo -e "${GREEN}Uninstallation complete!${NC}"
echo ""
echo "The Kinect XR runtime has been removed from your system."
