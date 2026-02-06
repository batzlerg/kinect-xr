# Getting Started with Kinect XR

Welcome to Kinect XR. Getting your Kinect working in the browser takes just 3 commands.

## Quick Start

```bash
# Step 1: Clone the repo
git clone https://github.com/batzlerg/kinect-xr.git && cd kinect-xr

# Step 2: Install dependencies and build
./scripts/setup.sh

# Step 3: Start everything (opens browser automatically)
./scripts/start.sh
```

That's it. Your browser will open with a live Kinect depth visualization.

### What Just Happened?

The setup script:
- Installed libfreenect via Homebrew (if needed)
- Built the C++ bridge server
- Prepared the web examples

The start script:
- Launched the WebSocket bridge server
- Started a local web server
- Opened your browser to the test page

### Shutting Down

When you're done, cleanly stop everything:

```bash
./scripts/stop.sh
```

This terminates the bridge server and web server processes.

---

## No Kinect? No Problem

Run in mock mode to explore the examples without hardware:

```bash
./scripts/start.sh --mock
```

Mock mode generates synthetic depth data so you can test the visualization pipeline.

---

## Developer Integration

Once you have the bridge running, connect from your own code:

### JavaScript/Browser

```javascript
import { KinectClient, KINECT } from './lib/kinect-client.js';

const kinect = new KinectClient();

// Receive depth frames
kinect.onDepthFrame = (depth, frameId) => {
  // depth is Uint16Array - values in millimeters (800-4000)
  const centerDepth = depth[320 + 240 * KINECT.WIDTH];
  console.log('Center pixel:', centerDepth, 'mm');
};

// Receive RGB frames
kinect.onRgbFrame = (imageData, frameId) => {
  // imageData ready for canvas.putImageData()
  ctx.putImageData(imageData, 0, 0);
};

await kinect.connect();
```

### P5.js

See the included examples:
- `web/examples/p5js/index.html` - 3D depth particle cloud
- `web/examples/p5js/depth-mask.html` - Interactive depth effects

### Three.js

See `web/examples/threejs/index.html` for a WebGL point cloud example.

### Constants

```javascript
KINECT.WIDTH        // 640
KINECT.HEIGHT       // 480
KINECT.MIN_DEPTH_MM // 800
KINECT.MAX_DEPTH_MM // 4000
```

---

## Manual Installation

If you prefer to set things up yourself, or the setup script doesn't work for your system:

### Prerequisites

**macOS:**
- Xcode Command Line Tools: `xcode-select --install`
- Homebrew: https://brew.sh
- CMake 3.20+: `brew install cmake`
- libfreenect: `brew install libfreenect`
- Bun (for web server): `brew install oven-sh/bun/bun`

### Build Steps

```bash
# Clone
git clone https://github.com/batzlerg/kinect-xr.git
cd kinect-xr

# Configure CMake
cmake -B build -S .

# Build
cmake --build build

# Verify build succeeded
./build/bin/kinect-bridge --help
```

### Running Manually

**Terminal 1 - Bridge Server:**
```bash
# With Kinect hardware (requires sudo for USB access on macOS)
sudo ./build/bin/kinect-bridge

# Or mock mode (no hardware required)
./build/bin/kinect-bridge --mock
```

**Terminal 2 - Web Server:**
```bash
cd web
bun run serve.js
```

**Terminal 3 - Open Browser:**
```bash
open http://localhost:3000
```

---

## Troubleshooting Setup Script

### "setup.sh: command not found"

Make sure you're in the kinect-xr directory and the script is executable:

```bash
cd kinect-xr
chmod +x scripts/setup.sh
./scripts/setup.sh
```

### "libfreenect not found" after setup

Homebrew may need you to restart your terminal:

```bash
# Close and reopen terminal, then:
brew --prefix libfreenect
# Should show: /opt/homebrew/opt/libfreenect (Apple Silicon)
# Or: /usr/local/opt/libfreenect (Intel)
```

### CMake configuration fails

If you see errors about missing dependencies:

```bash
# Ensure Homebrew packages are findable
export CMAKE_PREFIX_PATH="$(brew --prefix)"

# Re-run setup
./scripts/setup.sh
```

### Build errors with Google Test

If you have googletest installed via Homebrew and see conflicts:

```bash
brew uninstall googletest
./scripts/setup.sh
```

The project bundles its own GoogleTest - system installs can cause conflicts.

### USB permission errors (macOS)

The Kinect requires elevated privileges for USB access:

```bash
# Either run bridge with sudo
sudo ./build/bin/kinect-bridge

# Or use mock mode (no sudo needed)
./scripts/start.sh --mock
```

### "Connection failed" in browser

1. Is the bridge server running? Check Terminal output.
2. Is it listening on port 8765? Run `lsof -i :8765`
3. Check browser console (F12) for WebSocket errors.

### Low frame rate / choppy video

- Try receiving only depth: `kinect.setStreams(['depth'])`
- Close other applications using USB bandwidth
- Use a powered USB hub (Kinect draws significant power)

---

## Next Steps

- Explore the P5.js examples in `web/examples/p5js/`
- Build your own visualization using `KinectClient`
- Check `docs/ARCHITECTURE.md` for system design details
- Run the test suite: `./test.sh`

---

## Architecture Overview

```
Hardware:   Kinect Device
               |
Driver:     libfreenect (USB communication)
               |
Bridge:     kinect-bridge (C++ WebSocket server)
               |
Browser:    KinectClient.js --> Your App
```

The bridge server captures frames from the Kinect and streams them over WebSocket. Browser applications connect and receive real-time RGB + depth data.

For the full technical design, see `docs/ARCHITECTURE.md`.
