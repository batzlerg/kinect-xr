#!/usr/bin/env bash
# Compile the list-kinects utility

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Compiling list-kinects utility..."
g++ -std=c++17 list-kinects.cpp -o list-kinects -I/opt/homebrew/include -L/opt/homebrew/lib -lfreenect -lfreenect_sync

echo "✓ Compiled successfully"
echo ""
echo "Run with: sudo $SCRIPT_DIR/list-kinects"
echo "(sudo required for USB device access on macOS)"
