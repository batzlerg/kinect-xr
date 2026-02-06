# Metal Kinect Viewer Spike

Time-boxed exploration of Metal texture upload from Kinect frame data. This spike validates the graphics integration approach before implementing full OpenXR swapchain support in Phase 3.

## Purpose

Answer the question: "Can we efficiently get Kinect frames into Metal textures?"

## Building

```bash
cmake -B build -S .
cmake --build build
```

## Running

**Requires hardware** (Kinect 1 connected) and **elevated privileges** (USB access):

```bash
sudo ./build/MetalKinectViewer
```

## Testing

- **M1:** `test_m2.sh` - Kinect device integration
- **M3:** `test_m3.sh` - RGB texture display
- **M4:** `test_m4.sh` - RGB + depth side-by-side display
- **M5:** `profile_m5.sh` - Performance profiling

## Performance Results (M5)

### Frame Rate
- **Target:** >= 25 FPS (ideally 30 FPS)
- **Achieved:** [To be measured]

### CPU Usage
- **Texture upload thread:** [To be profiled]
- **Main rendering thread:** [To be profiled]
- **Total CPU %:** [To be profiled]

### GPU Usage
- **Texture copy overhead:** [To be profiled]
- **Fragment shader cost:** [To be profiled]
- **Total GPU %:** [To be profiled]

### Memory Usage
- **RGB texture:** 640x480x4 = 1.2 MB
- **Depth texture:** 640x480x2 = 600 KB
- **Total texture memory:** ~1.8 MB
- **Peak memory:** [To be profiled]

### Bottlenecks
[To be identified via Instruments]

## Findings

### What Worked
- BGRA8Unorm format for RGB texture
- R16Uint format for depth texture
- CPU-side RGB->BGRA conversion
- MTLTexture.replaceRegion() for upload
- Dispatch queue for async texture updates

### Issues Encountered
[To be documented]

### Recommendations for Phase 3
[To be documented after profiling]

## Code Structure

```
spike/metal/
├── CMakeLists.txt           # Build configuration
├── MetalKinectViewer/
│   ├── main.mm              # Cocoa app + Metal renderer
│   └── Shaders.metal        # Vertex/fragment shaders
├── test_*.sh                # Manual validation scripts
├── profile_m5.sh            # Profiling guide
└── README.md                # This file
```

## Key Patterns

### Texture Upload
```objc
// Async texture update on dispatch queue
dispatch_async(_textureUpdateQueue, ^{
    // Convert RGB->BGRA
    // Call [texture replaceRegion:...]
});
```

### Split-Screen Rendering
```objc
// Set viewport for left half
MTLViewport leftViewport = {0, 0, halfWidth, height, 0, 1};
[renderEncoder setViewport:leftViewport];
// Draw RGB...

// Set viewport for right half
MTLViewport rightViewport = {halfWidth, 0, halfWidth, height, 0, 1};
[renderEncoder setViewport:rightViewport];
// Draw depth...
```

### Depth Visualization
```metal
// Read uint depth value
uint depthValue = depthTexture.read(texCoord).r;
// Normalize and invert
float normalized = 1.0 - (float(depthValue) / 2047.0);
```

## Related Documentation
- See `docs/planning/metal-integration.md` for detailed findings
- See `specs/004-MetalSpike.md` for implementation log
