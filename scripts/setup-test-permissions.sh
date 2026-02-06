#!/bin/bash
# Setup passwordless sudo for Kinect XR test binaries
# This allows agents to run hardware tests without manual password entry
#
# SECURITY NOTE: This grants passwordless sudo to SPECIFIC binaries only.
# The binaries are fully specified by path, limiting exposure.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Sudoers file location
SUDOERS_FILE="/etc/sudoers.d/kinect-xr-tests"

# Get current user
CURRENT_USER=$(whoami)

echo "Kinect XR Test Permissions Setup"
echo "================================="
echo ""
echo "This script will configure passwordless sudo for test binaries."
echo ""
echo "User: $CURRENT_USER"
echo "Project: $PROJECT_DIR"
echo "Build: $BUILD_DIR"
echo ""
echo "The following binaries will be granted NOPASSWD access:"
echo "  - $BUILD_DIR/bin/unit_tests"
echo "  - $BUILD_DIR/bin/integration_tests"
echo "  - $BUILD_DIR/bin/runtime_tests"
echo "  - $BUILD_DIR/bin/kinect-bridge"
echo ""

# Check if already configured
if [ -f "$SUDOERS_FILE" ]; then
    echo "Sudoers file already exists: $SUDOERS_FILE"
    echo "Current contents:"
    sudo cat "$SUDOERS_FILE"
    echo ""
    read -p "Overwrite? (y/N): " confirm
    if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
        echo "Aborted."
        exit 0
    fi
fi

# Create sudoers content
SUDOERS_CONTENT="# Kinect XR test binaries - passwordless sudo for hardware access
# Created by setup-test-permissions.sh
# Project: $PROJECT_DIR

# Allow $CURRENT_USER to run test binaries without password
$CURRENT_USER ALL=(ALL) NOPASSWD: $BUILD_DIR/bin/unit_tests
$CURRENT_USER ALL=(ALL) NOPASSWD: $BUILD_DIR/bin/integration_tests
$CURRENT_USER ALL=(ALL) NOPASSWD: $BUILD_DIR/bin/runtime_tests
$CURRENT_USER ALL=(ALL) NOPASSWD: $BUILD_DIR/bin/kinect-bridge
"

echo "Creating sudoers file..."
echo ""

# Write to temp file first, then validate with visudo
TEMP_FILE=$(mktemp)
echo "$SUDOERS_CONTENT" > "$TEMP_FILE"

# Validate syntax
echo "Validating sudoers syntax..."
if ! sudo visudo -c -f "$TEMP_FILE" 2>/dev/null; then
    echo "ERROR: Invalid sudoers syntax!"
    rm "$TEMP_FILE"
    exit 1
fi

# Install the file
echo "Installing to $SUDOERS_FILE..."
sudo cp "$TEMP_FILE" "$SUDOERS_FILE"
sudo chmod 440 "$SUDOERS_FILE"
sudo chown root:wheel "$SUDOERS_FILE"
rm "$TEMP_FILE"

echo ""
echo "✅ Configuration complete!"
echo ""
echo "You can now run tests without password prompts:"
echo "  sudo $BUILD_DIR/bin/unit_tests"
echo "  sudo $BUILD_DIR/bin/integration_tests"
echo ""
echo "To remove this configuration later:"
echo "  sudo rm $SUDOERS_FILE"
