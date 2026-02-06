# Kinect Depth Sensing Web Integration: Project Summary

## Mental Model \& Technology Stack

Through our exploration, we've built a comprehensive understanding of the technology stack for integrating Kinect depth data with web applications:

### Hardware to Browser Stack

```
Device Level:     Kinect Hardware (USB Device with Depth Camera)
                           ↓
Driver Level:     libfreenect (Uses isochronous USB transfers)
                           ↓
Runtime Level:    OpenXR Runtime (Exposes standardized spatial interfaces)
                           ↓
Bridge Level:     WebSocket Bridge (Connects OpenXR to browsers)
                           ↓
Browser Level:    WebXR-compatible Applications (JavaScript/P5.js)
```


## Key Technology Components

### Current Status

1. **libfreenect**:
    - Mature C/C++ library for Kinect
    - Successfully installed and tested on macOS
    - Provides robust access to depth data streams
2. **WebXR Depth Sensing Module**:
    - Part of evolving WebXR standard
    - Currently has limited browser support:
        - Chrome/Edge: Partial implementation (versions 79-136)
        - Firefox/Safari: No current support
    - Designed primarily for AR headsets with integrated depth sensors
3. **OpenXR**:
    - More modular native standard than WebXR
    - Supports specialized device implementations through extensions
    - Possibly relevant extensions for Kinect integration (more research needed):
        - XR_MSFT_scene_understanding
        - XR_FB_depth_buffer
        - XR_FB_scene
4. **Browser WebXR Support**:
    - Chrome provides partial implementation of WebXR Depth Sensing
    - Current architecture expects depth sensors to be part of complete XR systems
    - Discovery mechanism relies on OS-level XR runtime registration

## Implementation Approach

Based on our discussion, we identified developing an OpenXR runtime for Kinect as the primary focus:

```
┌─────────────┐     ┌─────────────────────┐     ┌───────────────────┐
│   Kinect    │────▶│  Kinect OpenXR      │────▶│  OpenXR Apps      │
│  (Hardware) │     │     Runtime         │     │                   │
│             │     │                     │     │                   │
└─────────────┘     └─────────────────────┘     └───────────────────┘
                            │                             ▲
                            │                             │
                            ▼                             │
                    ┌─────────────────┐         ┌─────────────────┐
                    │  WebSocket      │─────────▶│ WebXR Polyfill │
                    │    Bridge       │         │  (in browser)   │
                    └─────────────────┘         └─────────────────┘
```


### OpenXR Runtime Implementation

1. Create a native OpenXR runtime that implements:
    - Core OpenXR API (session management, spaces)
    - Depth-specific extensions
    - Bridge between Kinect's data format and OpenXR expectations
    - Register with the OS as a valid XR runtime

### Separate WebSocket Bridge

1. Develop a separate application that:
    - Connects to the OpenXR runtime as a client
    - Streams OpenXR depth data via WebSockets to browsers
    - Handles protocol translation between OpenXR and WebXR formats

### WebXR Polyfill Adapter

1. Create a JavaScript library to:
    - Implement the WebXR Depth Sensing Module interfaces
    - Connect to the WebSocket bridge
    - Make Kinect depth data available through standard WebXR API

## Technical Challenges

1. **OpenXR Extension Implementation**: Implementing the depth sensing extensions correctly according to the OpenXR spec, especially mapping Kinect's specific depth format to OpenXR expectations.
2. **OS Runtime Registration**: Properly registering the OpenXR runtime with the operating system so it's discoverable by both native applications and potentially browsers with OpenXR backends.
3. **Depth Data Transformation**: Converting between Kinect's native depth coordinate system and the OpenXR spatial reference system.
4. **Performance Optimization**: Managing the high bandwidth requirements of streaming depth data (10MB/s) with minimal latency through the OpenXR runtime.
5. **WebXR Compatibility**: Designing the WebSocket bridge to properly translate between OpenXR's depth representation and WebXR Depth Sensing Module's expected formats.

## Advantages of This Approach

1. **Future Compatibility**: As browser WebXR implementations mature, they may begin using OpenXR as their backend, enabling direct Kinect integration.
2. **Broader Ecosystem Access**: The Kinect would be usable by any OpenXR-compatible application, not just web browsers.
3. **Clean Layering**: A separate WebSocket bridge can serve any OpenXR device, not just Kinect.
4. **Learning Value**: Development provides deeper understanding of spatial computing standards.

This approach focuses first on creating a standards-compliant OpenXR runtime for Kinect, making it broadly useful across spatial computing applications while providing a path to web integration through the WebSocket bridge and WebXR polyfill.