# Spec 009: WebSocket Bridge Server

**Status:** active
**Created:** 2026-02-06
**Branch:** main
**Depends on:** Spec 008 (Protocol)

## Problem Statement

The WebSocket protocol is defined (Spec 008), but we need an actual server implementation that reads from KinectDevice and streams frames to browser clients.

## Goal

Implement a C++ WebSocket server that:
1. Listens on `ws://localhost:8765/kinect`
2. Sends `hello` with capabilities on connection
3. Handles `subscribe`/`unsubscribe` messages
4. Streams RGB + depth frames at 30 Hz from KinectDevice
5. Manages multiple concurrent clients
6. Handles Kinect connect/disconnect gracefully

## Non-Goals

- TLS/HTTPS (localhost only)
- Authentication
- Remote access (bind to localhost only)
- Motor control (deferred)

## Technical Approach

### WebSocket Library: IXWebSocket

Selected for:
- Modern C++ (C++11+)
- CMake FetchContent support
- Built-in threading
- Handles WebSocket protocol details
- BSD license

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    BridgeServer                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  IXWebSocket Server (port 8765)                     │    │
│  │  - Connection handling                              │    │
│  │  - Message routing                                  │    │
│  └──────────────────────────┬──────────────────────────┘    │
│                             │                                │
│  ┌──────────────────────────▼──────────────────────────┐    │
│  │  Client Manager                                     │    │
│  │  - Track connected clients                          │    │
│  │  - Track subscriptions per client                   │    │
│  │  - Broadcast frames to subscribed clients           │    │
│  └──────────────────────────┬──────────────────────────┘    │
│                             │                                │
│  ┌──────────────────────────▼──────────────────────────┐    │
│  │  Frame Loop (30 Hz thread)                          │    │
│  │  - Read from KinectDevice frame cache               │    │
│  │  - Build binary messages (header + pixels)          │    │
│  │  - Broadcast to subscribed clients                  │    │
│  └──────────────────────────┬──────────────────────────┘    │
└─────────────────────────────┼───────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│  KinectDevice (existing Phase 1 code)                       │
│  - setDepthCallback() → updates frame cache                 │
│  - setVideoCallback() → updates frame cache                 │
└─────────────────────────────────────────────────────────────┘
```

### Threading Model

1. **Main thread:** Runs IXWebSocket server event loop
2. **Kinect callback thread:** libfreenect USB thread updates frame cache
3. **Frame broadcast thread:** 30 Hz loop reads cache and broadcasts

Synchronization via mutex on frame cache (already exists in runtime.h as FrameCache).

### Message Building

```cpp
// Binary frame: 8-byte header + pixel data
struct FrameHeader {
    uint32_t frameId;      // Little-endian
    uint16_t streamType;   // 0x0001 = RGB, 0x0002 = Depth
    uint16_t reserved;     // Set to 0
};

// RGB frame: 8 + 921600 = 921608 bytes
// Depth frame: 8 + 614400 = 614408 bytes
```

## Milestones

### M1: CMake Integration
- [x] Add IXWebSocket via FetchContent
- [x] Create bridge server library target
- [x] Verify builds successfully

### M2: Server Skeleton
- [x] Implement BridgeServer class
- [x] Handle WebSocket connections
- [x] Send hello message on connect
- [x] Parse subscribe/unsubscribe

### M3: Frame Streaming (Mock Data)
- [x] Implement 30 Hz broadcast loop
- [x] Build binary frame messages
- [x] Stream mock test pattern (no Kinect required)
- [x] Validate with test client

### M4: KinectDevice Integration
- [ ] Wire up frame cache from KinectDevice callbacks
- [ ] Stream live Kinect data
- [ ] Handle Kinect disconnect/reconnect
- [ ] Send error messages on device issues

### M5: Multi-Client Support
- [ ] Track multiple client connections
- [ ] Per-client subscription state
- [ ] Clean shutdown handling
- [ ] Status message broadcasting

### M6: Testing & Validation
- [ ] Unit tests for message building
- [ ] Integration test with test-client
- [ ] 5-minute stability test
- [ ] Memory leak check (sanitizers)

## Testing Strategy

**Unit Tests:**
- FrameHeader serialization
- JSON message building
- Client subscription state machine

**Integration Tests:**
- Full server + test client loop
- Multiple client connections
- Kinect disconnect simulation

**Manual Validation:**
- Verify with Node.js test client
- Benchmark sustained 30 Hz
- Check memory stability

## Files to Create

```
src/bridge/
├── bridge_server.h        # BridgeServer class declaration
├── bridge_server.cpp      # Implementation
├── frame_broadcaster.h    # 30 Hz frame loop
├── frame_broadcaster.cpp
└── client_manager.h       # Client tracking

tools/
└── kinect-bridge          # Standalone executable

tests/unit/
└── bridge_tests.cpp       # Unit tests
```

## Dependencies

- IXWebSocket (via FetchContent)
- KinectDevice (existing)
- nlohmann/json (for JSON messages, via FetchContent)

## Open Questions

1. ~~Which WebSocket library?~~ **Decision: IXWebSocket**
2. ~~How to handle slow clients?~~ **Decision: Drop frames, track dropped_frames count**

## Revision History

| Date | Change |
|------|--------|
| 2026-02-06 | Initial spec |
