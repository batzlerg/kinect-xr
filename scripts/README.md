# Testing & Diagnostic Scripts

Scripts for testing, validating, and diagnosing Kinect hardware.

---

## Quick Reference

### Utilities (No sudo required)

```bash
# Get USB info for device inventory
./capture-device-info.sh

# Quick check if Kinect is connected
./quick-device-check.sh
```

### Testing (Requires sudo)

```bash
# Fast motor capability test
sudo ./quick-motor-test.sh

# Comprehensive single device test (motor + LED + streams)
sudo ./test-device.sh

# Full test suite for all devices
sudo ./test-all-devices.sh

# Full device capabilities listing
sudo ./list-kinects
```

### Diagnostics (Requires sudo)

```bash
# Deep diagnostics for investigating issues
sudo ./diagnose-device.sh
```

---

## Script Details

### `capture-device-info.sh`

Extract USB enumeration data for device inventory.

**Usage:**
```bash
./capture-device-info.sh
```

**Output:** Formatted data ready to paste into `docs/DEVICE_INVENTORY.md`

**Requires sudo:** No

---

### `quick-device-check.sh`

Quick check for connected Kinect devices using USB enumeration.

**Usage:**
```bash
./quick-device-check.sh
```

**Output:** USB device listing and model identification guide

**Requires sudo:** No

---

### `quick-motor-test.sh`

Fast motor capability test using `freenect_sync_get_tilt_state()`.

**Usage:**
```bash
sudo ./quick-motor-test.sh
```

**Output:** Motor status, tilt angle, accelerometer data

**Requires sudo:** Yes (USB device access)

---

### `list-kinects`

Enumerate all connected Kinect devices with full capabilities.

**Usage:**
```bash
sudo ./list-kinects
```

**Output:** Device enumeration, model identification, motor/LED status

**Requires sudo:** Yes (USB device access)

**Note:** Compile from source with `./build-list-kinects.sh`

---

### `test-device.sh`

Comprehensive test for a single device (USB + motor + LED + streams).

**Usage:**
```bash
sudo ./test-device.sh
```

**Output:** Complete test results including integration tests

**Requires sudo:** Yes (USB device access)

---

### `test-all-devices.sh`

Complete test suite for all three devices. Same as `test-device.sh` but with enhanced formatting.

**Usage:**
```bash
sudo ./test-all-devices.sh
```

**Output:** Formatted test results suitable for documentation

**Requires sudo:** Yes (USB device access)

---

### `diagnose-device.sh`

Deep diagnostics for investigating device issues.

**Usage:**
```bash
sudo ./diagnose-device.sh
```

**Tests:**
- Motor control via tiltdemo
- Motor control via sync API
- Depth data range analysis

**Output:** Detailed diagnostic information

**Requires sudo:** Yes (USB device access)

---

## Build Scripts

### `build-list-kinects.sh`

Compile the `list-kinects` utility.

**Usage:**
```bash
./build-list-kinects.sh
```

**Output:** `list-kinects` executable

---

## macOS USB Access

All scripts requiring sudo need elevated privileges for USB device access (libusb limitation).

**One-time setup** for integration tests:
```bash
./setup-test-permissions.sh
```

This configures passwordless sudo for test binaries only.

**Run hardware tests:**
```bash
./run-hardware-tests.sh          # All tests
./run-hardware-tests.sh unit     # Unit tests only
./run-hardware-tests.sh integration  # Integration tests only
```

---

## Documentation

- Device specifications: [docs/DEVICE_INVENTORY.md](../docs/DEVICE_INVENTORY.md)
- Testing report: [docs/DEVICE_TESTING_REPORT.md](../docs/DEVICE_TESTING_REPORT.md)
