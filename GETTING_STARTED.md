# Getting Started with Kinect XR

Welcome! This guide will get you from zero to visualizing Kinect depth data in your browser in about 5 minutes.

## Quick Start (Creative Coders)

If you have a Kinect and just want to see P5.js examples running:

```bash
# 1. Clone and build
git clone https://github.com/batzlerg/kinect-xr.git
cd kinect-xr
brew install libfreenect
cmake -B build -S . && cmake --build build

# 2. Start the bridge server (requires sudo on macOS)
sudo ./build/bin/kinect_ws_bridge

# 3. In another terminal, start the web server
cd web && bun run serve.js

# 4. Open your browser
open http://localhost:3000/examples/p5js/
```

You should see a 3D particle cloud driven by your Kinect depth data.

**No Kinect hardware?** Use mock mode to explore the examples:

```bash
# No sudo needed for mock mode
./build/bin/kinect_ws_bridge --mock
```

---

## Prerequisites

### Hardware

| Item | Notes |
|------|-------|
| **Kinect for Xbox 360** (Model 1414) | The original Kinect. Kinect v2 and Azure Kinect are not supported. |
| **USB Adapter** | If your Mac lacks USB-A ports, you need a USB-A to USB-C adapter. Powered hubs recommended for stability. |
| **Power Supply** | Kinect requires 12V DC. Use the original AC adapter or a compatible power supply. |

### Software

| Requirement | Version | Installation |
|-------------|---------|--------------|
| **macOS** | 10.15+ (Catalina or later) | - |
| **Homebrew** | Latest | [brew.sh](https://brew.sh) |
| **CMake** | 3.20+ | `brew install cmake` |
| **C++ Compiler** | Clang (Xcode) | `xcode-select --install` |
| **libfreenect** | 0.7.x | `brew install libfreenect` |
| **Bun** (optional) | Latest | `brew install bun` (for web server) |

### Optional Tools

| Tool | Purpose |
|------|---------|
| **Node.js** | Alternative to Bun for web server |
| **Python 3** | Alternative web server: `python3 -m http.server 3000` |

---

## Installation

### Step 1: Clone the Repository

```bash
git clone https://github.com/batzlerg/kinect-xr.git
cd kinect-xr
```

**Expected output:**
```
Cloning into 'kinect-xr'...
```

### Step 2: Install libfreenect

```bash
brew install libfreenect
```

**Expected output:**
```
==> Downloading ...
==> Installing libfreenect
```

**Verify installation:**
```bash
brew info libfreenect
```

You should see version 0.7.x listed.

### Step 3: Build the Project

```bash
cmake -B build -S .
cmake --build build
```

**Expected output (first run):**
```
-- The CXX compiler identification is AppleClang ...
-- Configuring done
-- Generating done
-- Build files have been written to: .../kinect-xr/build
...
[100%] Built target kinect_ws_bridge
```

**Verify build:**
```bash
ls build/bin/
```

You should see: `kinect_ws_bridge`, `unit_tests`, `integration_tests`

---

## Running the Examples

### Step 1: Start the Bridge Server

The bridge server connects to your Kinect and streams data over WebSocket.

**With Kinect hardware (requires sudo on macOS):**
```bash
sudo ./build/bin/kinect_ws_bridge
```

**Expected output:**
```
[INFO] Kinect XR Bridge starting...
[INFO] Kinect device found
[INFO] WebSocket server listening on ws://localhost:8765/kinect
[INFO] Streaming RGB + Depth @ 30Hz
```

**Without hardware (mock mode):**
```bash
./build/bin/kinect_ws_bridge --mock
```

Mock mode generates synthetic depth patterns for testing.

### Step 2: Start the Web Server

In a **new terminal**:

```bash
cd kinect-xr/web
bun run serve.js
```

**Expected output:**
```
Serving on http://localhost:3000
```

**Alternative (Python):**
```bash
cd kinect-xr/web
python3 -m http.server 3000
```

### Step 3: Open in Browser

Navigate to:

| URL | Description |
|-----|-------------|
| `http://localhost:3000/test.html` | Raw RGB + depth visualization |
| `http://localhost:3000/examples/p5js/` | 3D depth particle cloud |
| `http://localhost:3000/examples/p5js/depth-mask.html` | Depth-based effects (mask, silhouette, heatmap) |

### What You Should See

**Test Page (`test.html`):**
- Two panels: RGB video on the left, grayscale depth map on the right
- Frame counters showing ~30 FPS
- Connection stats at the bottom

**Depth Particles (`examples/p5js/`):**
- 3D particle cloud where each point's position and color is driven by depth
- Move your mouse to rotate the view
- Closer objects appear as warmer colors (red/orange)

**Depth Mask (`examples/p5js/depth-mask.html`):**
- Interactive depth effects with mode selector
- Adjust the depth threshold with sliders
- Modes: Mask, Silhouette, Heatmap, Edges

---

## Developer Integration

### Using kinect-client.js

The browser client library provides a clean API for accessing Kinect data:

```javascript
import { KinectClient, KINECT } from './lib/kinect-client.js';

const kinect = new KinectClient();

// Receive RGB frames as ImageData (ready for canvas)
kinect.onRgbFrame = (imageData, frameId) => {
  ctx.putImageData(imageData, 0, 0);
};

// Receive depth as Uint16Array (values in millimeters)
kinect.onDepthFrame = (depth, frameId) => {
  // depth[i] = distance in mm (800-4000)
  const centerDepth = depth[320 + 240 * 640];
  console.log('Center pixel depth:', centerDepth, 'mm');
};

// Connection lifecycle
kinect.onConnect = (capabilities) => console.log('Connected!', capabilities);
kinect.onDisconnect = () => console.log('Disconnected');
kinect.onError = (err) => console.error('Error:', err);

// Connect to the bridge
await kinect.connect();

// Optimize bandwidth: receive only depth
kinect.setStreams(['depth']);

// Check statistics
const stats = kinect.getStats();
console.log('Frames received:', stats.depthFrames);

// Cleanup
kinect.disconnect();
```

### Constants

```javascript
KINECT.WIDTH        // 640
KINECT.HEIGHT       // 480
KINECT.MIN_DEPTH_MM // 800  (0.8 meters)
KINECT.MAX_DEPTH_MM // 4000 (4 meters)
KINECT.STREAM_RGB   // 'rgb'
KINECT.STREAM_DEPTH // 'depth'
```

### Raw WebSocket Protocol

For direct WebSocket integration (Three.js, Babylon.js, etc.):

```javascript
const ws = new WebSocket('ws://localhost:8765/kinect');
ws.binaryType = 'arraybuffer';

ws.onmessage = (event) => {
  if (typeof event.data === 'string') {
    const msg = JSON.parse(event.data);
    if (msg.type === 'hello') {
      // Server sends capabilities on connect
      ws.send(JSON.stringify({ type: 'subscribe', streams: ['rgb', 'depth'] }));
    }
  } else {
    // Binary frame with 8-byte header
    const header = new DataView(event.data, 0, 8);
    const frameId = header.getUint32(0, true);
    const streamType = header.getUint16(4, true);

    if (streamType === 0x0001) {
      // RGB: 921,600 bytes (640x480x3)
      const rgb = new Uint8Array(event.data, 8);
    } else if (streamType === 0x0002) {
      // Depth: 614,400 bytes (640x480x2)
      const depth = new Uint16Array(event.data, 8);
    }
  }
};
```

### API Documentation

- **Client Library:** [web/README.md](web/README.md) - Full API reference
- **WebSocket Protocol:** [specs/archive/008-WebSocketBridgeProtocol.md](specs/archive/008-WebSocketBridgeProtocol.md) - Message schemas, binary format

---

## Troubleshooting

### "Permission denied" or "could not open device"

**Problem:** macOS requires elevated privileges for USB device access.

**Solution:**
```bash
sudo ./build/bin/kinect_ws_bridge
```

For development, you can configure passwordless sudo for the binary:
```bash
./scripts/setup-test-permissions.sh
```

### "No Kinect device found"

**Check connections:**
1. Is the Kinect power supply connected? The LED should glow green.
2. Is the USB cable firmly connected?
3. For USB-C adapters: try a powered hub.

**Check detection:**
```bash
# List USB devices
system_profiler SPUSBDataType | grep -i xbox

# Should show: Xbox NUI Camera, Xbox NUI Motor
```

**Still not detected?** Try:
```bash
# Reset USB
sudo killall -STOP -c usbd
sudo killall -CONT -c usbd
```

### "WebSocket connection failed"

**Is the bridge server running?**
```bash
# Check if port 8765 is in use
lsof -i :8765
```

**Firewall blocking?**
Check System Settings > Network > Firewall and allow incoming connections for the bridge.

### Low FPS or dropped frames

**Reduce bandwidth:**
```javascript
// Receive only depth data
kinect.setStreams(['depth']);
```

**Check CPU usage:**
If the browser tab is using >100% CPU, reduce particle count in examples.

### Build errors with GoogleTest

**Problem:** System GoogleTest conflicts with bundled version.

**Solution:**
```bash
brew uninstall googletest
cmake -B build -S . --fresh
cmake --build build
```

---

## Browser Compatibility

| Browser | Status | Notes |
|---------|--------|-------|
| **Chrome** | Supported | Recommended for best performance |
| **Firefox** | Supported | Full WebSocket + WebGL support |
| **Safari** | Supported | May need to enable WebGL in Preferences |
| **Edge** | Supported | Chromium-based |

**Requirements:**
- WebSocket support (all modern browsers)
- WebGL 1.0+ (for 3D examples)
- ES Modules (for kinect-client.js)

---

## Next Steps

### Learn More

- **Product Vision:** [docs/PRD.md](docs/PRD.md) - Project goals and roadmap
- **System Architecture:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - Technical design, dual-path strategy

### Build Custom Integrations

1. **Three.js:** Use raw WebSocket protocol to feed PointCloud geometry
2. **Babylon.js:** Similar approach with particle systems
3. **React/Vue:** Wrap KinectClient in a custom hook/composable

### Explore the Codebase

```
kinect-xr/
├── web/
│   ├── lib/kinect-client.js  # Browser client library
│   ├── examples/p5js/        # P5.js creative coding examples
│   └── test.html             # Raw test page
├── src/
│   └── bridge/               # WebSocket bridge server (C++)
├── include/kinect_xr/
│   └── device.h              # Kinect device abstraction
└── specs/archive/            # Completed specifications
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Run tests: `./test.sh`
4. Submit a pull request

Bug reports and feature requests welcome at [GitHub Issues](https://github.com/batzlerg/kinect-xr/issues).

---

## Getting Help

- **GitHub Issues:** [github.com/batzlerg/kinect-xr/issues](https://github.com/batzlerg/kinect-xr/issues)
- **libfreenect community:** [OpenKinect/libfreenect](https://github.com/OpenKinect/libfreenect)

Happy hacking!
