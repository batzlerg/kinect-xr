# Spec 008: WebSocket Bridge Protocol

**Status:** complete
**Created:** 2026-02-06
**Branch:** main (no worktree needed for protocol design)

## Problem Statement

Chrome's WebXR implementation does not support OpenXR on macOS. To enable P5.js/Three.js creative coding with Kinect depth data, we need a bridge that streams Kinect frames from the native runtime to browser JavaScript via WebSocket.

## Goal

Design a frame streaming protocol that:
1. Streams Kinect RGB + depth at 30 Hz to browser clients
2. Uses JSON metadata + binary data for efficiency
3. Supports version negotiation for future compatibility
4. Handles connection lifecycle gracefully
5. Enables multiple concurrent browser clients

## Non-Goals

- Authentication/authorization (localhost only)
- Encryption (localhost WebSocket is sufficient)
- Bidirectional control (motor tilt deferred)
- Frame compression (premature optimization)

## Protocol Design

### Transport

- **Protocol:** WebSocket (RFC 6455)
- **Endpoint:** `ws://localhost:8765/kinect`
- **Port:** 8765 (above privileged range, unlikely conflict)

### Message Types

All messages are either:
- **Text frames:** JSON metadata/control messages
- **Binary frames:** Raw pixel data with 8-byte header

### Connection Lifecycle

```
Client                              Server
  |                                    |
  |-------- WebSocket Connect -------->|
  |                                    |
  |<------- hello (capabilities) ------|
  |                                    |
  |-------- subscribe (streams) ------>|
  |                                    |
  |<------- frame (RGB binary) --------|
  |<------- frame (depth binary) ------|
  |         ... 30 Hz loop ...         |
  |                                    |
  |-------- unsubscribe -------------->|
  |                                    |
  |<------- goodbye -------------------|
  |                                    |
```

### Message Schemas

#### 1. Server → Client: `hello`

Sent immediately after WebSocket connection established.

```json
{
  "type": "hello",
  "protocol_version": "1.0",
  "server": "kinect-xr-bridge",
  "capabilities": {
    "streams": ["rgb", "depth"],
    "rgb": {
      "width": 640,
      "height": 480,
      "format": "RGB888",
      "bytes_per_frame": 921600
    },
    "depth": {
      "width": 640,
      "height": 480,
      "format": "UINT16",
      "bits_per_pixel": 16,
      "bytes_per_frame": 614400,
      "min_depth_mm": 800,
      "max_depth_mm": 4000
    },
    "frame_rate_hz": 30
  }
}
```

#### 2. Client → Server: `subscribe`

Client requests which streams to receive.

```json
{
  "type": "subscribe",
  "streams": ["rgb", "depth"]
}
```

Allowed values: `["rgb"]`, `["depth"]`, `["rgb", "depth"]`

#### 3. Client → Server: `unsubscribe`

Stop receiving frames (but keep connection open).

```json
{
  "type": "unsubscribe"
}
```

#### 4. Server → Client: Binary Frame

Binary messages contain frame data with an 8-byte header:

```
Bytes 0-3: Frame ID (uint32, little-endian, monotonically increasing)
Bytes 4-5: Stream type (uint16, little-endian)
             0x0001 = RGB
             0x0002 = Depth
Bytes 6-7: Reserved (uint16, set to 0)
Bytes 8+:  Raw pixel data
```

**RGB data (921,600 bytes):** Row-major, RGB888 (3 bytes per pixel)
**Depth data (614,400 bytes):** Row-major, uint16 little-endian (2 bytes per pixel)

#### 5. Server → Client: `status`

Periodic status or error notification.

```json
{
  "type": "status",
  "kinect_connected": true,
  "frame_id": 12345,
  "dropped_frames": 0,
  "clients_connected": 2
}
```

#### 6. Server → Client: `error`

Error notification.

```json
{
  "type": "error",
  "code": "KINECT_DISCONNECTED",
  "message": "Kinect device was disconnected",
  "recoverable": true
}
```

Error codes:
- `KINECT_DISCONNECTED` - Device unplugged (recoverable: true)
- `KINECT_NOT_FOUND` - No device on startup (recoverable: false)
- `STREAM_FAILED` - USB transfer error (recoverable: true)
- `PROTOCOL_ERROR` - Invalid client message (recoverable: true)

#### 7. Server → Client: `goodbye`

Sent before server closes connection.

```json
{
  "type": "goodbye",
  "reason": "shutdown"
}
```

### Frame Synchronization

- RGB and depth frames arrive independently from Kinect hardware
- Server assigns same `frame_id` to temporally-closest RGB+depth pair
- Client should buffer and correlate by `frame_id`
- If streams desync by >2 frames, server sends `status` with `dropped_frames` count

### Bandwidth Analysis

At 30 Hz with both streams:
- RGB: 921,600 × 30 = 27.6 MB/s
- Depth: 614,400 × 30 = 18.4 MB/s
- **Total: ~46 MB/s** (368 Mbps)

This is well within localhost WebSocket capacity but notable for memory allocation.

### Client Implementation Notes

```javascript
// Minimal client example
const ws = new WebSocket('ws://localhost:8765/kinect');

ws.onmessage = (event) => {
  if (typeof event.data === 'string') {
    const msg = JSON.parse(event.data);
    if (msg.type === 'hello') {
      ws.send(JSON.stringify({ type: 'subscribe', streams: ['rgb', 'depth'] }));
    }
  } else {
    // Binary frame
    const header = new DataView(event.data, 0, 8);
    const frameId = header.getUint32(0, true);
    const streamType = header.getUint16(4, true);
    const pixelData = new Uint8Array(event.data, 8);

    if (streamType === 0x0001) {
      // RGB frame (921600 bytes)
    } else if (streamType === 0x0002) {
      // Depth frame (614400 bytes as Uint16Array)
      const depthData = new Uint16Array(event.data, 8);
    }
  }
};
```

## Milestones

### M1: Protocol Specification (This Document)
- [x] Define message types and schemas
- [x] Document binary frame format
- [x] Specify connection lifecycle
- [x] Calculate bandwidth requirements

### M2: Test Client Implementation
- [x] Create Node.js test client
- [x] Validate protocol against spec
- [x] Measure frame timing
- [ ] Test reconnection behavior (requires server)

### M3: Protocol Documentation
- [x] Add protocol to docs/ARCHITECTURE.md
- [x] Create example client snippet (in test-client.js)
- [x] Document error handling patterns

## Testing Strategy

**Unit Tests:**
- Message serialization/deserialization
- Binary header parsing
- Frame ID sequencing

**Integration Tests:**
- Full connection lifecycle (connect → subscribe → frames → unsubscribe → disconnect)
- Multiple client handling
- Kinect disconnect/reconnect (when hardware available)

**Performance Tests:**
- Sustained 30 Hz delivery over 5 minutes
- Memory stability (no leaks)
- CPU usage profiling

## Dependencies

- WebSocket library for C++ server (evaluate: Boost.Beast, libwebsockets, uWebSockets)
- Existing KinectDevice from Phase 1

## Open Questions

1. ~~Should we compress frames?~~ **Decision: No, premature optimization. 46 MB/s is fine for localhost.**
2. ~~Single binary message with both RGB+depth, or separate?~~ **Decision: Separate. Allows subscribing to depth-only for performance.**
3. ~~Port number?~~ **Decision: 8765 (memorable, unlikely conflict)**

## Revision History

| Date | Change |
|------|--------|
| 2026-02-06 | Initial protocol design |
