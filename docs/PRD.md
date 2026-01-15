# Kinect XR

## Purpose

OpenXR runtime implementation for Kinect 1 hardware, enabling old Kinect devices for modern AR/VR applications with a path to WebXR integration.

## Users

- **Graham**: Personal Kinect experiments with P5.js
- **XR developers**: OpenXR-compatible Kinect support

## Capabilities

### Hardware Interface
- Kinect device management via libfreenect
- Depth and RGB camera access
- Isochronous USB transfer

### OpenXR Runtime
- Standards-compliant OpenXR implementation
- Kinect as XR input device

### WebXR Bridge (Future)
- WebSocket application for browser access
- W3C WebXR candidate proposal support

## Non-Goals

- Kinect 2/Azure Kinect support
- Full body tracking (v1 scope)
- Commercial distribution
