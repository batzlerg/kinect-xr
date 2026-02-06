#!/usr/bin/env bash
# Register Kinect XR runtime for development
# Usage: source scripts/register-runtime.sh

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
MANIFEST_PATH="$REPO_DIR/manifest/kinect_xr.json"

export XR_RUNTIME_JSON="$MANIFEST_PATH"

echo "Kinect XR runtime registered at: $XR_RUNTIME_JSON"
echo "OpenXR applications will now use the Kinect XR runtime."
