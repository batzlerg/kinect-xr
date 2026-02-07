#!/usr/bin/env bash
# Quick motor capability test using freenect_sync_get_tilt_state()
# Requires sudo for USB device access

set -e

if [ "$EUID" -ne 0 ]; then
    echo "⚠️  This script requires sudo for USB device access"
    echo "Run with: sudo $0"
    exit 1
fi

echo "Testing motor control..."
echo ""

cat > /tmp/quick_motor_test.c << 'EOF'
#include <libfreenect/libfreenect_sync.h>
#include <stdio.h>

int main() {
    freenect_raw_tilt_state *state;
    int ret = freenect_sync_get_tilt_state(&state, 0);

    if (ret != 0) {
        printf("❌ Motor control FAILED (error %d)\n", ret);
        printf("\nDebugging steps:\n");
        printf("1. Check USB cable connection\n");
        printf("2. Check power adapter is plugged in\n");
        printf("3. Try different USB port\n");
        printf("4. Check physical label - is it really Model 1414?\n");
        return 1;
    }

    printf("✅ Motor control WORKING\n");
    printf("   Angle: %.1f degrees\n", freenect_get_tilt_degs(state));
    printf("   Accel: X=%d Y=%d Z=%d\n",
           state->accelerometer_x,
           state->accelerometer_y,
           state->accelerometer_z);

    freenect_sync_stop();
    return 0;
}
EOF

gcc /tmp/quick_motor_test.c -o /tmp/quick_motor_test \
    -I/opt/homebrew/include -L/opt/homebrew/lib \
    -lfreenect -lfreenect_sync 2>&1 | grep -v "warning:"

/tmp/quick_motor_test
