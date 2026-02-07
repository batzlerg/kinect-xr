/**
 * @file list-kinects.cpp
 * @brief Enumerate all connected Kinect devices and display detailed info
 *
 * Compiles and runs standalone to identify Kinect models, firmware, and serial numbers.
 * Useful for determining which devices need special firmware loading (models 1473/1517).
 */

#include <libfreenect/libfreenect.h>
#include <libfreenect/libfreenect_sync.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <unistd.h>

// USB Vendor/Product IDs for different Kinect models
struct KinectModel {
    uint16_t product_id;
    const char* name;
    const char* notes;
};

static const KinectModel KNOWN_MODELS[] = {
    { 0x02ae, "Kinect for Xbox 360 (1414)", "Original model, no firmware required" },
    { 0x02bf, "Kinect for Xbox 360 (1473)", "Requires audio firmware for motor/LED" },
    { 0x02c2, "Kinect for Xbox 360 (1517)", "Requires audio firmware for motor/LED" },
    { 0x02b0, "Kinect for Windows (1414)", "Official Windows version" }
};

const char* getModelName(uint16_t product_id) {
    for (const auto& model : KNOWN_MODELS) {
        if (model.product_id == product_id) {
            return model.name;
        }
    }
    return "Unknown Kinect model";
}

const char* getModelNotes(uint16_t product_id) {
    for (const auto& model : KNOWN_MODELS) {
        if (model.product_id == product_id) {
            return model.notes;
        }
    }
    return "No additional information";
}

bool needsFirmware(uint16_t product_id) {
    return product_id == 0x02bf || product_id == 0x02c2;
}

void printDeviceInfo(freenect_context* ctx, int index, struct freenect_device_attributes* attr) {
    freenect_device* dev;

    std::cout << "\n========================================\n";
    std::cout << "Device #" << index << "\n";
    std::cout << "========================================\n";

    // Show camera serial from attributes
    std::cout << "Camera Serial:    " << (attr->camera_serial ? attr->camera_serial : "N/A") << "\n";

    // Open device to test motor/LED capabilities
    if (freenect_open_device(ctx, &dev, index) < 0) {
        std::cout << "ERROR: Could not open device for testing\n";
        return;
    }

    // Test LED control first (device is already open)
    int led_result = freenect_set_led(dev, LED_GREEN);
    if (led_result == 0) {
        std::cout << "LED Control:      YES\n";
        // Reset LED
        freenect_set_led(dev, LED_OFF);
    } else {
        std::cout << "LED Control:      NO (may need firmware)\n";
    }

    // Close device before using sync API
    freenect_close_device(dev);

    // Try to get tilt state using sync API (more reliable for motor control)
    freenect_raw_tilt_state* state;
    int tilt_result = freenect_sync_get_tilt_state(&state, index);
    if (tilt_result == 0 && state) {
        double angle = freenect_get_tilt_degs(state);
        std::cout << "Motor Support:    YES (current angle: " << std::fixed << std::setprecision(1) << angle << "°)\n";
        std::cout << "Accelerometer:    X=" << state->accelerometer_x
                  << " Y=" << state->accelerometer_y
                  << " Z=" << state->accelerometer_z << "\n";
    } else {
        std::cout << "Motor Support:    NO (may need firmware)\n";
    }

    // Stop sync API to release device
    freenect_sync_stop();
}

int main() {
    std::cout << "Kinect Device Enumeration Tool\n";
    std::cout << "===============================\n\n";

    freenect_context* ctx;

    if (freenect_init(&ctx, NULL) < 0) {
        std::cerr << "ERROR: Failed to initialize libfreenect\n";
        return 1;
    }

    // Select all subdevices
    freenect_select_subdevices(ctx, (freenect_device_flags)(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));

    // Get device attributes list
    struct freenect_device_attributes* attr_list = nullptr;
    int num_devices = freenect_list_device_attributes(ctx, &attr_list);

    if (num_devices < 0) {
        std::cerr << "ERROR: Failed to list devices\n";
        freenect_shutdown(ctx);
        return 1;
    }

    std::cout << "Found " << num_devices << " Kinect device(s)\n";

    if (num_devices == 0) {
        std::cout << "\nNo Kinect devices detected.\n";
        std::cout << "Make sure:\n";
        std::cout << "  - Kinect is plugged into USB\n";
        std::cout << "  - Kinect power adapter is connected\n";
        std::cout << "  - You have USB permissions (may need sudo on macOS)\n";
        freenect_shutdown(ctx);
        return 0;
    }

    // Enumerate all devices
    struct freenect_device_attributes* current = attr_list;
    int index = 0;
    while (current != nullptr) {
        printDeviceInfo(ctx, index, current);
        current = current->next;
        index++;
    }

    // Free the attribute list
    freenect_free_device_attributes(attr_list);

    std::cout << "\n========================================\n";
    std::cout << "Model Identification Guide\n";
    std::cout << "========================================\n\n";

    std::cout << "Physical identification (look for label on Kinect):\n";
    std::cout << "  - Model 1414: Original Xbox 360 Kinect (2010)\n";
    std::cout << "  - Model 1473: Revised Xbox 360 Kinect (2011+)\n";
    std::cout << "  - Model 1517: Later Xbox 360 Kinect variant\n\n";

    std::cout << "Firmware requirements:\n";
    std::cout << "  - Model 1414: No firmware needed ✓\n";
    std::cout << "  - Model 1473: Requires audio firmware for motor/LED control\n";
    std::cout << "  - Model 1517: Requires audio firmware for motor/LED control\n\n";

    std::cout << "If Motor/LED show 'NO' or 'UNKNOWN' above:\n";
    std::cout << "  → You likely have model 1473 or 1517\n";
    std::cout << "  → See: https://github.com/OpenKinect/libfreenect/issues/451\n\n";

    freenect_shutdown(ctx);
    return 0;
}
