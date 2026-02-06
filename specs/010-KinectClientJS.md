# Spec 010: Kinect Client JS Library

**Status:** active
**Created:** 2026-02-06
**Branch:** main
**Depends on:** Spec 008 (Protocol), Spec 009 (Server)

## Problem Statement

Browser applications need a simple way to receive Kinect RGB + depth data from the WebSocket bridge server without dealing with raw protocol details.

## Goal

Create a lightweight JavaScript library that:
1. Connects to the Kinect bridge server
2. Handles protocol handshake automatically
3. Provides simple callbacks for RGB and depth frames
4. Converts RGB888 to ImageData for canvas rendering
5. Exposes raw Uint16Array for depth processing

## Non-Goals

- Full WebXR Device API polyfill (descoped - too complex)
- Three.js integration (separate example)
- TypeScript types (defer)

## API Design

```javascript
import { KinectClient, KINECT } from './kinect-client.js';

const kinect = new KinectClient('ws://localhost:8765/kinect');

// Callbacks
kinect.onRgbFrame = (imageData, frameId) => { /* ImageData for canvas */ };
kinect.onDepthFrame = (depth, frameId) => { /* Uint16Array[307200] */ };
kinect.onConnect = (capabilities) => { /* server capabilities */ };
kinect.onDisconnect = () => { /* cleanup */ };
kinect.onError = (error) => { /* {code, message, recoverable} */ };

// Connect (returns Promise)
await kinect.connect();

// Configure streams
kinect.setStreams(['depth']); // depth only

// Statistics
const stats = kinect.getStats();
// { rgbFrames, depthFrames, droppedFrames, bytesReceived }

// Cleanup
kinect.disconnect();

// Constants
KINECT.WIDTH  // 640
KINECT.HEIGHT // 480
KINECT.MIN_DEPTH_MM // 800
KINECT.MAX_DEPTH_MM // 4000
```

## Milestones

### M1: Core Library
- [x] KinectClient class with WebSocket handling
- [x] Protocol handshake (hello → subscribe)
- [x] Binary frame parsing (8-byte header)
- [x] RGB888 → RGBA conversion for ImageData
- [x] Depth Uint16Array passthrough

### M2: Test Page
- [x] HTML test page with canvas rendering
- [x] RGB video display
- [x] Depth visualization (grayscale)
- [x] Connection status and stats display

### M3: Documentation
- [ ] README with usage examples
- [ ] JSDoc comments
- [ ] P5.js integration example

## Files Created

```
web/
├── lib/
│   └── kinect-client.js   # Main library (ES module)
├── test.html              # Test page with canvas
└── serve.js               # Simple static file server
```

## Testing

**Manual testing:**
1. Start bridge server: `./build/bin/kinect-bridge --mock`
2. Start web server: `cd web && bun run serve.js`
3. Open http://localhost:3000 in browser
4. Click Connect - should see animated RGB + depth

**Expected behavior:**
- RGB canvas shows moving color gradient
- Depth canvas shows grayscale waves
- Stats show ~30 FPS for both streams
- No dropped frames on localhost

## Revision History

| Date | Change |
|------|--------|
| 2026-02-06 | Initial implementation (M1-M2 complete) |
