# Kinect XR Web Integration

Browser-based visualization of Kinect RGB + depth data.

## Prerequisites

The web server requires **Bun** (not Node.js):

```bash
brew install bun
```

The `serve.js` file uses Bun-specific APIs (`Bun.serve()`, `Bun.file()`) and will not work with Node.js.

## Quick Start

```bash
# Terminal 1: Start the bridge server (mock mode for testing)
cd /path/to/kinect-xr
./build/bin/kinect-bridge --mock

# Terminal 2: Serve web files
cd web
bun run serve.js

# Open browser
open http://localhost:3000
```

## Structure

```
web/
├── index.html             # Examples landing page
├── lib/
│   └── kinect-client.js   # Browser library for Kinect data
├── examples/
│   ├── threejs/
│   │   └── index.html     # 3D depth point cloud
│   ├── rgb-depth/
│   │   └── index.html     # Raw RGB + depth display
│   └── p5js/
│       ├── index.html     # 3D depth particles
│       └── depth-mask.html # Depth masking effects
├── serve.js               # Simple static file server
└── README.md
```

## Using kinect-client.js

```javascript
import { KinectClient, KINECT } from './lib/kinect-client.js';

const kinect = new KinectClient();

// Get RGB frames as ImageData (ready for canvas)
kinect.onRgbFrame = (imageData, frameId) => {
  ctx.putImageData(imageData, 0, 0);
};

// Get depth as Uint16Array (values in millimeters)
kinect.onDepthFrame = (depth, frameId) => {
  // depth[i] = distance in mm (800-4000)
  const centerDepth = depth[320 + 240 * 640];
  console.log('Center pixel depth:', centerDepth, 'mm');
};

// Connect to bridge
await kinect.connect();

// Only receive depth (saves bandwidth)
kinect.setStreams(['depth']);

// Check stats
const stats = kinect.getStats();
console.log('Frames received:', stats.depthFrames);

// Cleanup
kinect.disconnect();
```

## Constants

```javascript
KINECT.WIDTH        // 640
KINECT.HEIGHT       // 480
KINECT.MIN_DEPTH_MM // 800
KINECT.MAX_DEPTH_MM // 4000
KINECT.STREAM_RGB   // 'rgb'
KINECT.STREAM_DEPTH // 'depth'
```

## Examples

Visit the landing page at `http://localhost:3000` to browse all examples.

### Three.js Point Cloud (`/examples/threejs/`)

Interactive 3D point cloud visualization of depth data. Rotate and zoom to explore the scene.

### RGB + Depth Viewer (`/examples/rgb-depth/`)

Side-by-side view of raw RGB video and depth visualization with connection stats.

### P5.js Depth Particles (`/examples/p5js/`)

3D particle cloud driven by depth data. Each pixel becomes a sphere in 3D space, colored by depth.

### Depth Mask (`/examples/p5js/depth-mask.html`)

Interactive depth-based effects:
- **Mask:** Show RGB only within depth threshold
- **Silhouette:** White cutout on black
- **Heatmap:** Depth as color gradient
- **Edges:** Depth discontinuity detection

## With Real Kinect Hardware

```bash
# Requires sudo on macOS for USB access
sudo ./build/bin/kinect-bridge
```

## Troubleshooting

**"Connection failed"**
- Is the bridge server running? (`./build/bin/kinect-bridge --mock`)
- Check browser console for WebSocket errors

**No video/depth showing**
- Click the Connect button
- Check that bridge is sending frames (look at terminal output)

**Low FPS**
- Try `kinect.setStreams(['depth'])` to receive only depth
- Reduce particle count in examples
