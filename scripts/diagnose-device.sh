#!/usr/bin/env bash
# Deep diagnostics for investigating Kinect device issues
# Tests motor control, depth data range, and hardware capabilities
# Requires sudo for USB device access

set -e

if [ "$EUID" -ne 0 ]; then
    echo "⚠️  This script requires sudo for USB device access"
    echo "Run with: sudo $0"
    exit 1
fi

echo "════════════════════════════════════════════════════════"
echo "Kinect Device Diagnostic Tool"
echo "════════════════════════════════════════════════════════"
echo ""

echo "Test 1: Motor Control via freenect-tiltdemo"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Attempting to read current tilt angle and accelerometer..."
echo ""

# Try tiltdemo (it's interactive but we can send commands)
timeout 3 freenect-tiltdemo 0 2>&1 | head -30 || true

echo ""
echo ""
echo "Test 2: Motor Control via libfreenect sync API"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Create a quick test program
cat > /tmp/motor_test.c << 'EOF'
#include <libfreenect/libfreenect.h>
#include <libfreenect/libfreenect_sync.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Testing motor control with sync API...\n\n");

    // Try to get current tilt state
    freenect_raw_tilt_state *state;
    int ret = freenect_sync_get_tilt_state(&state, 0);

    if (ret != 0) {
        printf("❌ Failed to get tilt state (error: %d)\n", ret);
        printf("   This suggests motor subdevice is not responding\n\n");

        printf("Possible causes:\n");
        printf("  1. Device needs firmware (Model 1473/1517)\n");
        printf("  2. Motor not properly initialized\n");
        printf("  3. USB power issue\n");
        printf("  4. Motor hardware failure\n\n");
        return 1;
    }

    printf("✅ Motor communication successful!\n\n");

    double angle = freenect_get_tilt_degs(state);
    printf("Current tilt angle: %.1f degrees\n", angle);
    printf("Accelerometer: X=%d Y=%d Z=%d\n",
           state->accelerometer_x,
           state->accelerometer_y,
           state->accelerometer_z);

    freenect_tilt_status_code status = freenect_get_tilt_status(state);
    printf("Motor status: ");
    switch(status) {
        case TILT_STATUS_STOPPED: printf("STOPPED\n"); break;
        case TILT_STATUS_LIMIT:   printf("LIMIT REACHED\n"); break;
        case TILT_STATUS_MOVING:  printf("MOVING\n"); break;
        default: printf("UNKNOWN (%d)\n", status);
    }

    printf("\nTesting tilt command...\n");
    ret = freenect_sync_set_tilt_degs(10, 0);
    if (ret != 0) {
        printf("❌ Failed to set tilt angle (error: %d)\n", ret);
        return 1;
    }
    printf("✅ Tilt command sent successfully\n");

    sleep(1);

    // Read state again
    ret = freenect_sync_get_tilt_state(&state, 0);
    if (ret == 0) {
        angle = freenect_get_tilt_degs(state);
        status = freenect_get_tilt_status(state);
        printf("New angle: %.1f degrees, status: %d\n", angle, status);
    }

    // Reset to 0
    printf("\nResetting to 0 degrees...\n");
    freenect_sync_set_tilt_degs(0, 0);

    freenect_sync_stop();
    return 0;
}
EOF

gcc /tmp/motor_test.c -o /tmp/motor_test -I/opt/homebrew/include -L/opt/homebrew/lib -lfreenect -lfreenect_sync
/tmp/motor_test

echo ""
echo ""
echo "Test 3: Depth Data Range Analysis"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Capturing depth frame and analyzing values..."
echo ""

# Create depth analysis program
cat > /tmp/depth_test.c << 'EOF'
#include <libfreenect/libfreenect_sync.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t *depth = NULL;
    uint32_t timestamp;

    printf("Capturing depth frame...\n");
    int ret = freenect_sync_get_depth((void**)&depth, &timestamp, 0, FREENECT_DEPTH_11BIT);

    if (ret != 0) {
        printf("❌ Failed to get depth frame (error: %d)\n", ret);
        return 1;
    }

    printf("✅ Depth frame captured (timestamp: %u)\n\n", timestamp);

    // Analyze depth values
    int count_total = 640 * 480;
    int count_zero = 0;
    int count_valid = 0;
    int count_out_of_range = 0;
    uint16_t min_val = 65535;
    uint16_t max_val = 0;

    for (int i = 0; i < count_total; i++) {
        uint16_t val = depth[i];

        if (val == 0) {
            count_zero++;
        } else if (val <= 2047) {
            count_valid++;
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        } else {
            count_out_of_range++;
            if (count_out_of_range <= 5) {
                // Show first few out-of-range values
                printf("Out-of-range value at pixel %d: %u\n", i, val);
            }
        }
    }

    printf("\nDepth Statistics:\n");
    printf("  Total pixels:       %d\n", count_total);
    printf("  Zero (no depth):    %d (%.1f%%)\n", count_zero, 100.0 * count_zero / count_total);
    printf("  Valid (1-2047):     %d (%.1f%%)\n", count_valid, 100.0 * count_valid / count_total);
    printf("  Out of range:       %d (%.1f%%)\n", count_out_of_range, 100.0 * count_out_of_range / count_total);
    printf("  Min valid value:    %u\n", min_val);
    printf("  Max valid value:    %u\n", max_val);

    if (count_out_of_range > 0) {
        printf("\n⚠️  WARNING: Found %d pixels with values > 2047\n", count_out_of_range);
        printf("   Kinect depth should be 11-bit (0-2047)\n");
        printf("   Values > 2047 may indicate:\n");
        printf("     - Sensor noise/glitches\n");
        printf("     - USB packet corruption\n");
        printf("     - Hardware issue\n");
        printf("     - libfreenect parsing bug\n");

        if (count_out_of_range < count_total * 0.01) {
            printf("\n   Impact: LOW (< 1%% of pixels affected)\n");
            printf("   Likely safe to ignore or clamp to 2047\n");
        } else {
            printf("\n   Impact: HIGH (%.1f%% of pixels affected)\n",
                   100.0 * count_out_of_range / count_total);
            printf("   May indicate serious hardware/USB issue\n");
        }
    } else {
        printf("\n✅ All depth values within valid 11-bit range\n");
    }

    freenect_sync_stop();
    return 0;
}
EOF

gcc /tmp/depth_test.c -o /tmp/depth_test -I/opt/homebrew/include -L/opt/homebrew/lib -lfreenect_sync
/tmp/depth_test

echo ""
echo ""
echo "════════════════════════════════════════════════════════"
echo "Diagnostic Complete"
echo "════════════════════════════════════════════════════════"
echo ""
