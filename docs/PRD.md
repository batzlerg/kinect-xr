# Kinect XR - Product Requirements Document

## Purpose

OpenXR runtime implementation for Kinect 1 hardware, enabling legacy Kinect devices for modern AR/VR applications with a standards-compliant path to WebXR integration.

## Problem Statement

The original Kinect sensor (2010-2013) was groundbreaking depth-sensing hardware that became obsolete when Microsoft discontinued support. Millions of these devices exist with no modern software pathway. Meanwhile, the XR ecosystem has standardized on OpenXR, but no runtime exists for Kinect.

**Specific pain point:** Using Kinect depth data in web applications (P5.js, Three.js) requires custom drivers and non-standard protocols. WebXR provides a standard API, but browsers expect OpenXR-compatible runtimes.

## Users

### Primary User
**Graham** - Hardware hacker who wants to use Kinect 1 for P5.js creative coding experiments without writing custom protocol bridges.

### Secondary Users
**XR Developers** - Anyone wanting to repurpose legacy Kinect hardware for OpenXR-compatible applications.

## Capabilities

### Phase 1: Device Integration (MVP)

| Capability | Description | Priority |
|------------|-------------|----------|
| Device discovery | Enumerate connected Kinect devices | P0 |
| Device initialization | Open and configure Kinect via libfreenect | P0 |
| Depth streaming | Continuous depth frame capture (640x480, 11-bit) | P0 |
| RGB streaming | Continuous RGB frame capture (640x480) | P0 |
| Resource management | Clean initialization/shutdown, no leaks | P0 |
| Error handling | Clear error codes and recovery paths | P0 |

### Phase 2: OpenXR Runtime

| Capability | Description | Priority |
|------------|-------------|----------|
| Instance management | `xrCreateInstance`, `xrDestroyInstance` | P0 |
| Session management | Create/begin/end sessions | P0 |
| Swapchain management | Color and depth buffer handling | P0 |
| View configuration | Monoscopic camera view setup | P0 |
| Frame timing | Synchronization with Kinect frame rate | P1 |
| Extension discovery | Report supported extensions | P1 |

### Phase 3: Depth Extensions

| Capability | Description | Priority |
|------------|-------------|----------|
| XR_KHR_composition_layer_depth | Depth buffer submission | P0 |
| Depth value transformation | Convert 11-bit Kinect to OpenXR format | P0 |
| Frame synchronization | Align depth and color frames | P1 |

### Phase 4: Data Pipeline

| Capability | Description | Priority |
|------------|-------------|----------|
| Coordinate mapping | Kinect space to OpenXR space | P0 |
| Depth normalization | Scale depth values for OpenXR | P0 |
| Performance optimization | Handle 10MB/s throughput | P1 |

### Phase 5: Runtime Registration

| Capability | Description | Priority |
|------------|-------------|----------|
| macOS registration | `/usr/local/share/openxr/1/active_runtime.json` | P0 |
| Environment variable support | `XR_RUNTIME_JSON` for app-specific selection | P1 |
| Manifest creation | Proper runtime manifest for discovery | P0 |

### Phase 6: Motor Control Extension (Optional)

| Capability | Description | Priority |
|------------|-------------|----------|
| XR_KINECT_motor_control | Custom vendor extension | P2 |
| Tilt angle control | -31 to +31 degrees | P2 |
| Action bindings | Semantic actions for motor control | P2 |

## Delivered: WebSocket Bridge (Parallel Path)

**Status: Complete (2026-02-05)**

The WebSocket bridge is a **permanent parallel path** to the OpenXR runtime, not a temporary workaround or stepping stone. Chrome will never support OpenXR on macOS due to architectural limitations (requires D3D11, no Metal binding). Browser-based XR applications must use the WebSocket bridge.

| Component | Description | Status |
|-----------|-------------|--------|
| WebSocket Bridge Server | C++ server streaming RGB/depth over WebSocket | Complete |
| KinectClient.js | Browser client library for connecting to bridge | Complete |
| P5.js Examples | Creative coding examples (depth field, silhouette) | Complete |

**Usage:**
```bash
# Start the bridge server
./build/bin/kinect_ws_bridge

# Or in mock mode (no hardware required)
./build/bin/kinect_ws_bridge --mock
```

Browser applications connect via `ws://localhost:8765/kinect` and receive 30Hz depth/RGB streams.

See `docs/ARCHITECTURE.md` for the dual-path strategy explaining when to use the bridge vs. the OpenXR runtime.

## Non-Goals

| Explicitly Out of Scope | Rationale |
|------------------------|-----------|
| Kinect 2 / Azure Kinect | Different protocols, libfreenect2 needed |
| Full body tracking | Complex ML pipeline, separate project |
| Skeletal tracking | Kinect SDK feature, not in libfreenect |
| Commercial distribution | Personal/hobby project |
| Windows support | macOS focus initially, extensible later |
| Audio capture | Kinect microphone array not in scope |

## Success Criteria

### Phase 1 Complete When:
- [x] Can enumerate Kinect devices
- [x] Can initialize device with configuration
- [x] Can start/stop depth and RGB streams
- [x] Frame data accessible via callbacks
- [x] All unit tests pass without hardware
- [x] Integration tests pass with hardware
- [x] No resource leaks (verified with sanitizers)

### Phase 2+ Ready When:
- [ ] Phase 1 complete
- [ ] OpenXR SDK integrated
- [ ] Architecture document updated with OpenXR layer design
- [ ] Spike/POC validates Metal integration on macOS

## Technical Constraints

| Constraint | Reason |
|------------|--------|
| C++17 | Modern but compatible with libfreenect |
| macOS primary | Development machine, Apple Silicon |
| libfreenect 0.x | Only stable Kinect 1 driver |
| No OpenGL | Metal preferred for macOS |
| CMake | Cross-platform build system |

## Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| libfreenect | 0.7.x | Kinect USB communication |
| OpenXR SDK | 1.0.x | Runtime API definitions |
| GoogleTest | 1.13.0 | Testing framework |
| Metal | macOS native | Graphics backend |

## Timeline

**No fixed deadlines** - Personal project with flexible timeline.

Estimated effort (not calendar time):
- Phase 1: Device Integration - Complete (2026-02-05)
- WebSocket Bridge: Complete (2026-02-05) - Parallel path for browser applications
- Phase 2: OpenXR Runtime - Significant effort, enables native XR applications
- Phases 3-5: Medium each
- Phase 6: Optional, defer

## Revision History

| Date | Change |
|------|--------|
| 2025-04 | Initial project setup |
| 2026-02 | PRD formalized during compliance restructure |
| 2026-02-05 | WebSocket bridge moved from deferred to delivered; documented as permanent parallel path |
