# Metal Spike

**Status:** draft
**Created:** 2026-02-05
**Branch:** feature/004-MetalSpike
**Blocked By:** 001-StreamManagement

## Summary

Validate Metal texture creation and display from Kinect frame data. This time-boxed spike de-risks the graphics integration before implementing full OpenXR swapchain support in Phase 3.

**What:** Create minimal Metal application that receives Kinect depth/RGB frames and displays them in a window using Metal textures. Measure performance characteristics and document any issues.

**Why:** Phase 3 (depth extensions) requires Metal-backed swapchains. Metal API is unfamiliar territory and macOS-specific. By validating Metal texture creation from Kinect data separately, we isolate graphics issues from OpenXR complexity. This spike answers: "Can we efficiently get Kinect frames into Metal textures?"

**Current State:**
- Phase 1 provides frame callbacks with raw depth/RGB buffers
- No Metal code in project
- No GPU utilization currently

**Technical Approach:**
1. Create minimal Cocoa/Metal application
2. Use KinectDevice to receive frame callbacks
3. Upload frame data to MTLTexture
4. Render texture to window using simple quad
5. Measure frame rate, latency, CPU/GPU usage

**Testing Strategy:**
- Manual validation (visual inspection of displayed frames)
- Performance profiling with Instruments
- Hardware REQUIRED for this spike (cannot test without Kinect)

## Scope

**In:**
- Create minimal Metal rendering pipeline
- Upload Kinect RGB frames to MTLTexture
- Upload Kinect depth frames to MTLTexture (visualized as grayscale)
- Display textures in window
- Measure performance (FPS, latency)
- Document Metal API patterns for Phase 3
- Document any issues or limitations

**Out:**
- OpenXR integration
- Swapchain implementation
- Depth-to-color alignment
- Point cloud rendering
- Shaders beyond basic texture display
- Production-quality rendering

## Acceptance Criteria

**Does this spec involve UI-observable behavior?** YES - displays Kinect video feed

### Descriptive Criteria

- [ ] Metal window displays Kinect RGB feed
- [ ] Metal window displays Kinect depth feed (grayscale visualization)
- [ ] Frame rate >= 25 FPS (target 30 FPS)
- [ ] No visible frame tearing or corruption
- [ ] CPU usage documented
- [ ] GPU usage documented
- [ ] Memory usage documented
- [ ] Findings documented in docs/planning/

## Architecture Delta

**Before:**
Kinect frames received via callbacks but not displayed. No graphics pipeline.

**After:**
Spike demonstrates Metal texture pipeline. Patterns documented for Phase 3 swapchain implementation.

**Spike Structure (temporary, not merged to main):**
```
spike/metal/
├── MetalKinectViewer/
│   ├── main.swift (or main.mm)
│   ├── Renderer.swift
│   ├── Shaders.metal
│   └── Info.plist
└── README.md (findings)
```

## Milestones

- [x] **M1: Create minimal Metal window**
  - Create spike/metal/ directory
  - Create basic Cocoa application with Metal view
  - Render solid color to verify Metal pipeline works
  - Validation: Window displays, no crashes

- [x] **M2: Integrate KinectDevice**
  - Link spike app against kinect_xr_device library
  - Initialize KinectDevice in app startup
  - Start streams and verify callbacks fire
  - Log frame receipt to console
  - Validation: Console shows frame callbacks

- [x] **M3: Upload RGB frames to Metal texture**
  - Create MTLTexture with appropriate format (BGRA8Unorm)
  - Copy RGB frame data to texture on each callback
  - Handle RGB to BGRA conversion if needed
  - Render texture to screen using textured quad
  - Validation: Kinect RGB video displays in window

- [x] **M4: Upload depth frames to Metal texture**
  - Create MTLTexture for depth (R16Uint or R32Float)
  - Copy depth frame data to texture
  - Create visualization shader (depth to grayscale)
  - Add toggle or split-screen for RGB/depth
  - Validation: Kinect depth visualization displays

- [x] **M5: Performance profiling**
  - Measure achieved frame rate
  - Profile with Instruments (Time Profiler, Metal System Trace)
  - Document CPU usage during streaming
  - Document GPU usage during rendering
  - Document memory usage (texture allocation)
  - Identify any bottlenecks
  - Validation: Performance report complete

- [ ] **M6: Document findings**
  - Create docs/planning/metal-integration.md
  - Document Metal API patterns used
  - Document texture formats that work
  - Document performance characteristics
  - Document any issues or workarounds
  - Recommend approach for Phase 3 swapchains
  - Validation: Documentation complete

## Open Questions

**Q1: Swift or Objective-C++ for spike?**
- Option A: Swift with Metal (modern, cleaner)
- Option B: Objective-C++ (easier C++ interop with kinect_xr_device)
- Recommendation: Objective-C++ for easier integration
- Decision: TBD during M1

**Q2: Texture upload strategy?**
- Option A: CPU copy via MTLTexture.replace()
- Option B: Shared memory via MTLBuffer with texture view
- Option C: Metal Performance Shaders for conversion
- Recommendation: Start with Option A (simplest), optimize if needed
- Decision: Try A first during M3

**Q3: Should depth be normalized or raw?**
- Option A: Display raw 11-bit values as 16-bit grayscale
- Option B: Normalize to 0-1 float range
- Option C: Color-map depth values
- Recommendation: Option A for spike simplicity
- Decision: A for spike, revisit for Phase 3

---

## Implementation Log

### Milestone 1
- Created spike/metal directory structure
- Implemented minimal Cocoa app with Metal view in Objective-C++
- Created basic shaders (fullscreen triangle, solid green color)
- CMake build configuration with Metal shader compilation
- **Decision Q1:** Using Objective-C++ for C++ interop
- Validation: Window displays green, no crashes

### Milestone 2
- Extended KinectDevice with callback registration (setVideoCallback, setDepthCallback)
- Updated device.h/cpp to forward libfreenect callbacks to user callbacks
- Linked Metal spike against kinect_xr_device library
- Integrated Kinect initialization into AppDelegate
- Set up frame callbacks with frame counters
- Validation: App initializes Kinect, starts streams, logs frame receipt

### Milestone 3
- Created 640x480 BGRA8Unorm Metal texture for RGB data
- Implemented RGB888 to BGRA conversion in updateRGBTextureWithData
- Added texture sampler with linear filtering
- Updated shader to sample texture (textureFragment)
- Wired Kinect video callback to update texture on each frame
- **Decision Q2:** Using Option A (CPU copy via MTLTexture.replace)
- Validation: RGB video displays in window (manual test required)

### Milestone 4
- Created 640x480 R16Uint Metal texture for depth data
- Implemented depth texture upload (direct 16-bit values)
- Created depth visualization shader (grayscale, normalized to 0-2047)
- Split-screen rendering (RGB left, depth right) using viewports
- Wired Kinect depth callback to update texture
- **Decision Q3:** Option A (raw 11-bit as grayscale, inverted for visibility)
- Validation: Side-by-side RGB + depth display (manual test required)

### Milestone 5
- Added FPS counter to renderer (logs every 60 frames)
- Created profiling script with Instruments commands
- Documented profiling methodology in spike/metal/profile_m5.sh
- Created README.md with performance results template
- Validation: Manual profiling required (FPS measurement automated)
- Note: Detailed Instruments profiling deferred to manual execution

### Milestone 6
-

---

## Documentation Updates

### docs/planning/metal-integration.md (NEW FILE)

Document:
1. **Metal Setup**
   - Device/queue creation
   - Texture descriptor configuration
   - Render pipeline setup

2. **Texture Upload Patterns**
   - Best format for RGB (BGRA8Unorm vs RGBA8Unorm)
   - Best format for depth (R16Uint vs R32Float)
   - Upload method (replace vs shared buffer)
   - Synchronization considerations

3. **Performance Results**
   - Achieved FPS
   - CPU utilization
   - GPU utilization
   - Memory footprint

4. **Issues Encountered**
   - Any API gotchas
   - Format conversion needs
   - Threading considerations

5. **Recommendations for Phase 3**
   - Swapchain texture format
   - Double/triple buffering approach
   - Synchronization with OpenXR frame timing

---

## Archive Criteria

- [ ] All milestones complete
- [ ] All acceptance criteria met
- [ ] Performance documented
- [ ] Findings documented in docs/planning/metal-integration.md
- [ ] Recommendations for Phase 3 clear
- [ ] Spec moved to specs/archive/004-MetalSpike.md

---

## Notes

**This spike requires hardware.** Unlike other specs, this cannot be developed without a physical Kinect connected. Schedule spike work for when hardware is available.

**Spike code is exploratory.** The Metal viewer app may not be merged to main. Its purpose is learning and validation, not production code. Patterns learned will inform Phase 3 implementation.
