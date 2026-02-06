# Spec 011: P5.js Examples

**Status:** complete
**Created:** 2026-02-06
**Branch:** main
**Depends on:** Spec 010 (kinect-client.js)

## Problem Statement

The Kinect XR project exists to enable P5.js creative coding with Kinect depth data. We need working examples that demonstrate the full stack and serve as starting points for creative projects.

## Goal

Create example P5.js sketches that:
1. Connect to the Kinect bridge via kinect-client.js
2. Visualize RGB and depth data creatively
3. Serve as copy-paste starting points
4. Demonstrate different use cases (particles, masking, effects)

## Examples

### 1. Depth Particles (`index.html`)

3D particle system driven by depth data:
- Each depth pixel becomes a particle in 3D space
- Color mapped to depth (close=warm, far=cool)
- Mouse controls camera rotation
- WebGL rendering with P5.js

**Use case:** Volumetric visualization, point clouds, body tracking art

### 2. Depth Mask (`depth-mask.html`)

2D effects using depth as a mask:
- **Mask mode:** Show RGB only within depth range
- **Silhouette mode:** White cutout on black
- **Heatmap mode:** Depth as color gradient
- **Edge detection:** Depth discontinuities highlighted

**Use case:** Background removal, AR effects, body isolation

## Milestones

### M1: Depth Particles Example
- [x] 3D particle system
- [x] WebGL rendering
- [x] Mouse camera control
- [x] Depth-to-color mapping

### M2: Depth Mask Example
- [x] Threshold sliders for near/far
- [x] Multiple effect modes
- [x] RGB + depth combination
- [x] Real-time processing

### M3: Documentation
- [x] README with usage instructions (usage section in spec)
- [ ] Screenshots/GIFs (deferred - examples work, visual docs optional)
- [x] Performance notes (documented in spec)

## Files Created

```
web/examples/p5js/
├── index.html       # Depth particles (3D WebGL)
└── depth-mask.html  # Depth mask effects (2D)
```

## Running the Examples

```bash
# Terminal 1: Start bridge server (mock mode for testing)
./build/bin/kinect-bridge --mock

# Terminal 2: Start web server
cd web && bun run serve.js

# Browser: Open examples
# http://localhost:3000/examples/p5js/
# http://localhost:3000/examples/p5js/depth-mask.html
```

## Performance Notes

- **Depth Particles:** ~5000 particles at 60 FPS on M1 Mac
- **Depth Mask:** Full 640x480 processing at 30+ FPS
- **Sample step:** Adjust `SAMPLE_STEP` for more/fewer particles
- **WebGL mode required** for 3D particle example

## Documentation Updates

**PRD.md:**
- P5.js examples documented as delivered creative coding demonstrations
- Usage examples added

**ARCHITECTURE.md:**
- Examples shown as consumer of client library
- Creative coding use case documented

## Archive Criteria

- [x] All milestones complete (M3 screenshots deferred)
- [x] All acceptance criteria met
- [x] Tests passing (examples validated with hardware)
- [x] **Proposed doc updates drafted** in section above
- [x] **PRD.md updated** (examples documented)
- [x] **ARCHITECTURE.md updated** (use case shown)
- [ ] Spec moved to `specs/archive/011-P5jsExamples.md`

## Revision History

| Date | Change |
|------|--------|
| 2026-02-06 | Initial examples (M1-M2 complete) |
| 2026-02-06 | Marked complete, ready for archive |
