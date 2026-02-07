# KinectMotorControlWebAPI

**Status:** draft
**Created:** 2026-02-06
**Branch:** feature/014-KinectMotorControlWebAPI

## Summary

**What:** Expose Kinect motor control (tilt, LED, accelerometer) via WebSocket API, enabling web-based PTZ-style control of the Kinect sensor.

**Why:** Motor control is Phase 6 (Deferred) in the project roadmap but provides valuable functionality for XR applications requiring dynamic sensor positioning. The existing WebSocket bridge (ws://localhost:8765/kinect) handles depth/RGB streams but excludes motor control. This spec implements the motor API layer.

**Approach:** Extend KinectDevice class with motor control methods wrapping libfreenect motor API. Extend WebSocket protocol with motor commands/events. Add rate limiting (500ms min interval) to prevent hardware damage. Support model detection for firmware loading on 1473/1517 models.

**Technical Foundation (from libfreenect):**
- Tilt control: `freenect_set_tilt_degs()` supports -27 to +27 degrees (absolute positioning)
- Status queries: `freenect_get_tilt_state()`, `freenect_get_tilt_status()` (STOPPED/MOVING/LIMIT)
- LED control: `freenect_set_led()` supports 6 states (off, green, red, yellow, blink variants)
- Accelerometer: `freenect_get_mks_accel()` provides gravity-corrected X/Y/Z values
- All calls are blocking; status polling at 100-200ms intervals during movement

**Constraints:**
- Position-based control only (not continuous PTZ speed control)
- Blocking libfreenect calls require async wrapper pattern
- Rate limiting mandatory to prevent motor burnout
- Models 1473/1517 require runtime firmware loading

## Scope

**In:**
- Extend `KinectDevice` class with motor control methods (setTiltAngle, getTiltAngle, setLED, getMotorStatus, getAccelerometer)
- Extend WebSocket protocol with motor commands (`motor.setTilt`, `motor.setLed`, `motor.reset`, `motor.getStatus`)
- Extend WebSocket protocol with motor events (`motor.status`, `motor.error`)
- Implement server-side rate limiting (500ms min interval between tilt commands)
- Add angle clamping (-27 to +27 degrees with soft limits)
- Poll motor status during movement (100-200ms interval)
- Queue or reject commands that violate rate limits (reject with error preferred)
- Model detection for 1473/1517 firmware loading
- Update client SDK (kinect-client.js) with motor control methods
- Unit tests for device layer methods
- Integration tests for WebSocket commands (require hardware)

**Out:**
- Continuous PTZ speed-based control (hardware limitation - position-based only)
- Motor calibration routines (defer to future spec)
- Multi-Kinect motor coordination (out of scope)
- GUI motor control interface (client applications implement their own UI)
- Audio motor firmware (separate system, not controlled via motor API)

## Acceptance Criteria

### Descriptive Criteria

- [ ] Device layer compiles and passes unit tests
- [ ] WebSocket bridge handles motor commands without crashes
- [ ] Rate limiting prevents commands faster than 500ms interval
- [ ] Angle values clamped to -27/+27 degree range
- [ ] LED states correctly mapped to libfreenect constants
- [ ] Accelerometer returns gravity-corrected values
- [ ] Client SDK exposes motor control methods
- [ ] Integration tests pass with connected Kinect hardware
- [ ] Documentation updated (ARCHITECTURE.md, client SDK README)

### Integration Test Criteria (Hardware Required)

- [ ] `motor.setTilt` moves motor to specified angle and returns status
- [ ] `motor.setTilt` with out-of-range angle returns clamped value
- [ ] `motor.setLed` changes LED state correctly
- [ ] `motor.getStatus` returns current angle, tilt status, and accelerometer data
- [ ] Rate limit violation returns error (not silently queued)
- [ ] Motor status events stream during movement (MOVING -> STOPPED)

## Architecture Delta

**Before:**
```
KinectDevice (device.h/device.cpp)
  - connect(), disconnect()
  - startDepthStream(), startColorStream()
  - No motor control methods

WebSocket Bridge (bridge_server.cpp)
  - Handles: depth.start, depth.stop, color.start, color.stop
  - No motor message handlers

Client SDK (kinect-client.js)
  - startDepth(), startColor(), onDepthFrame(), onColorFrame()
  - No motor methods
```

**After:**
```
KinectDevice (device.h/device.cpp)
  - connect(), disconnect()
  - startDepthStream(), startColorStream()
  + setTiltAngle(degrees) -> DeviceError
  + getTiltAngle() -> float
  + setLED(LEDState) -> DeviceError
  + getMotorStatus() -> MotorStatus (angle, tilt_status, accel_x/y/z)
  + getAccelerometer() -> AccelData (x, y, z in m/s^2)

WebSocket Bridge (bridge_server.cpp)
  - Handles: depth.start, depth.stop, color.start, color.stop
  + Handles: motor.setTilt, motor.setLed, motor.reset, motor.getStatus
  + Emits: motor.status (angle, status), motor.error (code, message)
  + Rate limiter: 500ms min interval per command type

Client SDK (kinect-client.js)
  - startDepth(), startColor(), onDepthFrame(), onColorFrame()
  + setTilt(angle) -> Promise<MotorStatus>
  + setLED(state) -> Promise<void>
  + getMotorStatus() -> Promise<MotorStatus>
  + onMotorStatus(callback) -> unsubscribe function
```

**Data Flow:**
```
Web Client
    |
    | WebSocket: motor.setTilt { angle: 15 }
    v
Bridge Server
    |
    | Rate limit check (500ms since last tilt command?)
    | If violated: send motor.error { code: "RATE_LIMITED", message: "..." }
    |
    | Clamp angle to [-27, 27]
    v
KinectDevice.setTiltAngle(15)
    |
    | freenect_set_tilt_degs(dev, 15)
    | Poll freenect_get_tilt_status() every 100ms until STOPPED
    v
Bridge Server
    |
    | Emit motor.status { angle: 15, status: "STOPPED" } to all clients
    v
Web Client
```

## Milestones

### Device Layer

- [ ] **M1: Define motor types and enums** - Add LEDState enum, MotorStatus struct, TiltStatus enum to device.h
- [ ] **M2: Implement motor control methods** - Add setTiltAngle(), getTiltAngle(), setLED(), getMotorStatus(), getAccelerometer() to device.cpp using libfreenect API
- [ ] **M3: Unit tests for device layer** - Test angle clamping, LED state mapping, error handling (no hardware required for unit tests)

### WebSocket Protocol

- [x] **M4: Define motor protocol messages** - Add motor command/event types to protocol documentation
- [x] **M5: Implement motor message handlers** - Add handlers for motor.setTilt, motor.setLed, motor.reset, motor.getStatus in bridge server
- [x] **M6: Implement rate limiting** - Add 500ms rate limiter for motor commands, return motor.error on violation
- [ ] **M7: Implement status polling** - Poll motor status during movement, emit motor.status events to connected clients

### Client SDK

- [ ] **M8: Add motor methods to client SDK** - Implement setTilt(), setLED(), getMotorStatus(), onMotorStatus() in kinect-client.js
- [ ] **M9: Update client SDK documentation** - Add motor control examples to README

### Testing and Documentation

- [ ] **M10: Integration tests** - Hardware tests for motor control via WebSocket
- [ ] **M11: Update ARCHITECTURE.md** - Document motor control flow and API

## Open Questions

**Resolved during research:**

1. **Rate limiting strategy?** -> Reject with error (not queue). Clients should handle RATE_LIMITED error and retry.

2. **Firmware loading for 1473/1517?** -> Use `freenect_upload_firmware()` if `freenect_get_device_id()` returns 1473 or 1517. Handle FirmwareLoadError if upload fails.

3. **Blocking call handling?** -> Motor commands run synchronously in device layer. Bridge server can optionally run in background thread if latency becomes problematic.

**Open:**

1. **Should motor.reset return to 0 degrees or last known position?** -> Suggest 0 degrees (level position)

2. **Should accelerometer data be included in every motor.status event, or separate query?** -> Suggest separate getAccelerometer() to reduce message size; include in getMotorStatus() for convenience

---

## Implementation Log

### Milestone 1: Define motor types and enums
- Added `LEDState` enum, `TiltStatus` enum, `MotorStatus` struct to `device.h`
- Extended `DeviceError` enum with `MotorControlFailed`, `InvalidParameter`

### Milestone 2: Implement motor control methods
- Implemented `setTiltAngle()`, `getTiltAngle()`, `setLED()`, `getMotorStatus()` in `device.cpp`
- All methods wrap libfreenect motor API calls
- Angle clamping to [-27, 27] implemented

### Milestone 3: Unit tests for device layer
- Added 5 motor unit tests in `device_unit_test.cpp`
- All tests pass without hardware (graceful skip logic)

### Milestone 4: Define motor protocol messages
- Added motor command types: `motor.setTilt`, `motor.setLed`, `motor.reset`, `motor.getStatus`
- Added motor event types: `motor.status`, `motor.error`
- Updated hello message with motor capabilities

### Milestone 5: Implement motor message handlers
- Implemented `handleMotorSetTilt()`, `handleMotorSetLed()`, `handleMotorReset()`, `handleMotorGetStatus()`
- Added routing in `onMessage()` for all motor command types
- Implemented `sendMotorStatus()`, `sendMotorError()` helpers
- LED state string-to-enum mapping

### Milestone 6: Implement rate limiting
- Added `lastMotorCommand_` timestamp tracking with `motorMutex_`
- 500ms rate limit enforced in `handleMotorSetTilt()` and `handleMotorReset()`
- Returns `motor.error` with code `RATE_LIMITED` on violation

### Milestone 7: Implement status polling
-

### Milestone 8: Add motor methods to client SDK
-

### Milestone 9: Update client SDK documentation
-

### Milestone 10: Integration tests
-

### Milestone 11: Update ARCHITECTURE.md
-

---

## Technical Reference

### libfreenect Motor API

```c
// Tilt control (blocking)
int freenect_set_tilt_degs(freenect_device *dev, double angle);
// Returns 0 on success, <0 on error
// Angle range: -27 to +27 degrees

// Tilt state query
freenect_raw_tilt_state* freenect_get_tilt_state(freenect_device *dev);
int freenect_update_tilt_state(freenect_device *dev);
double freenect_get_tilt_degs(freenect_raw_tilt_state *state);
freenect_tilt_status_code freenect_get_tilt_status(freenect_raw_tilt_state *state);
// Status codes: TILT_STATUS_STOPPED, TILT_STATUS_LIMIT, TILT_STATUS_MOVING

// LED control
int freenect_set_led(freenect_device *dev, freenect_led_options option);
// Options: LED_OFF, LED_GREEN, LED_RED, LED_YELLOW,
//          LED_BLINK_GREEN, LED_BLINK_RED_YELLOW

// Accelerometer
void freenect_get_mks_accel(freenect_raw_tilt_state *state,
                            double* x, double* y, double* z);
// Returns gravity-corrected values in m/s^2 (9.8 = 1g)
```

### WebSocket Protocol Messages

**Commands (client -> server):**

```json
// Set tilt angle
{ "type": "motor.setTilt", "angle": 15 }

// Set LED state
{ "type": "motor.setLed", "state": "green" }
// States: "off", "green", "red", "yellow", "blink_green", "blink_red_yellow"

// Reset to level position
{ "type": "motor.reset" }

// Query current status
{ "type": "motor.getStatus" }
```

**Events (server -> client):**

```json
// Status update (sent during movement and after commands)
{
  "type": "motor.status",
  "angle": 15.2,
  "status": "STOPPED",  // or "MOVING", "LIMIT"
  "accelerometer": { "x": 0.1, "y": 0.2, "z": 9.8 }
}

// Error response
{
  "type": "motor.error",
  "code": "RATE_LIMITED",  // or "OUT_OF_RANGE", "MOTOR_LIMIT", "HARDWARE_ERROR"
  "message": "Minimum 500ms between tilt commands"
}
```

### Client SDK API

```javascript
const client = new KinectClient();

// Motor control
await client.setTilt(15);  // Returns MotorStatus when complete
await client.setLED('green');  // Returns void on success
const status = await client.getMotorStatus();  // { angle, status, accelerometer }

// Subscribe to motor status events
const unsubscribe = client.onMotorStatus((status) => {
  console.log(`Angle: ${status.angle}, Status: ${status.status}`);
});
```

---

## Documentation Updates

### docs/ARCHITECTURE.md Updates

Add motor control section documenting:
- Motor control data flow (client -> bridge -> device -> libfreenect)
- Rate limiting implementation (500ms interval)
- Status polling during movement (100-200ms)
- LED state mapping to libfreenect constants

### web/README.md Updates

Add motor control examples:
- Basic tilt control with setTilt()
- LED state control with setLED()
- Status monitoring with onMotorStatus()
- Error handling for RATE_LIMITED responses

---

## Archive Criteria

**Complete these BEFORE moving spec to archive:**

- [ ] All milestones complete
- [ ] All acceptance criteria met
- [ ] Tests passing (unit + integration with hardware)
- [ ] **Proposed doc updates drafted** in section above (based on actual implementation)
- [ ] **PRD.md updated** - Add motor control to Phase 6 features (now implemented)
- [ ] **ARCHITECTURE.md updated** - Motor control flow, rate limiting, protocol
- [ ] Spec moved to `specs/archive/014-KinectMotorControlWebAPI.md`
