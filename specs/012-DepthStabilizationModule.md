# DepthStabilizationModule

**Status:** complete
**Created:** 2026-02-06
**Branch:** feature/012-DepthStabilizationModule

## Summary

**Research-Only Spec:** This spec documents research into depth camera stabilization techniques for potential future implementation in Kinect XR. No implementation has been performed.

**What:** Investigation of real-time depth stabilization techniques suitable for Kinect v1 sensor data at 30 FPS within XR latency constraints (33-66ms acceptable).

**Why:** Kinect depth sensors exhibit temporal noise and flickering due to IR pattern correlation uncertainties, quantization errors, and signal degradation at distance. Stabilization could improve depth data quality for XR applications, but must operate within strict real-time constraints.

**Research Findings:**

**libfreenect Capabilities:** libfreenect provides raw depth data without built-in filtering. libfreenect2 (Kinect v2) implements bilateral and edge-aware filters in depth packet processors, but these are not available for Kinect v1 hardware.

**Technique Analysis:**
Five major approaches were evaluated based on academic literature and industry implementations (Intel RealSense, academic papers on Kinect filtering):

1. **Temporal Median Filter** - Excellent noise reduction (>2x) but introduces unacceptable latency (66-133ms for median-of-5/9 frames) for XR applications.

2. **Temporal Exponential Moving Average** - Intel RealSense approach. Single-frame memory, <1ms processing time, tunable smoothing via alpha parameter, edge preservation via delta threshold. Best performance-quality balance.

3. **Bilateral Filter** - Edge-preserving spatial smoothing. No temporal latency but computationally expensive (reduces framerate to 12-15fps on CPU). Requires GPU acceleration for real-time performance.

4. **Joint Bilateral Filter (Color-Guided)** - Uses RGB data to guide depth filtering. Superior edge preservation but requires aligned color/depth streams and assumes color-depth correspondence.

5. **Hybrid Spatial + Temporal Pipeline** - Intel RealSense production approach combining decimation, depth-to-disparity conversion, spatial filtering, temporal filtering, and hole filling. Best quality but highest complexity.

**Key Best Practices Identified:**
- Process in disparity space (inverse depth) for uniform noise characteristics
- Temporal filtering only for static/slow-moving scenes (causes motion blur on dynamic content)
- Maintain separate filter state per stream source (switching invalidates history)
- Clip maximum depth to application range (removes invalid pixels at sensor limits)
- Validate depth values before filtering (skip FREENECT_DEPTH_MM_NO_VALUE pixels)
- Consider GPU acceleration for spatial filters (embarrassingly parallel)

**Performance Constraints:**
- Target: 30 FPS (33.3ms per frame)
- Acceptable latency: 33-66ms (1-2 frames)
- Computational budget: <10ms per frame for filtering (leaves 23ms for OpenXR processing)

**Recommended Approach (if implemented):**
Temporal Exponential Moving Average (Intel RealSense-style) offers the best balance for Phase 1. Algorithm:
```
For each pixel:
  if |depth_curr - depth_prev| < delta_threshold:
    depth_out = alpha * depth_curr + (1-alpha) * depth_prev  // Smooth within surfaces
  else:
    depth_out = depth_curr  // Preserve edges
```

Configuration:
- Alpha (smoothing strength): 0.4 default, range 0.0-1.0
- Delta (edge threshold): 20mm default
- Memory: Single previous frame buffer per stream (~614KB for 640x480x2 bytes)
- Processing time: <1ms per frame (Intel RealSense measurements)

This approach avoids the latency penalty of median filtering while providing configurable noise reduction suitable for XR applications.

## Scope

**In:**
- Research on temporal filtering techniques (median, exponential moving average)
- Research on spatial filtering techniques (bilateral, joint bilateral)
- Analysis of libfreenect capabilities and limitations
- Performance analysis for 30 FPS real-time operation
- Best practices from academic literature and industry implementations (Intel RealSense)
- Recommended approach with implementation outline

**Out:**
- Actual implementation (deferred - this is research only)
- GPU acceleration research (focus on CPU-based approaches)
- Multi-Kinect interference patterns
- Advanced hole-filling algorithms
- Machine learning-based approaches

## Acceptance Criteria

**Research-only spec - Descriptive criteria only.**

### Descriptive Criteria

- [x] libfreenect capabilities documented (no built-in filtering)
- [x] Five major stabilization techniques analyzed with pros/cons/complexity
- [x] Performance constraints identified (30 FPS, <10ms budget, 33-66ms latency)
- [x] Best practices documented from academic and industry sources
- [x] Recommended approach selected with implementation outline
- [x] Sources cited for all research findings
- [x] YAML research output generated

## Architecture Delta

**Before:**
No depth stabilization. Raw depth data from libfreenect passed directly to OpenXR runtime. Temporal noise and flickering present in depth stream.

**After (if implemented):**
Optional DepthStabilizer class inserted into depth processing pipeline:
```
KinectDevice → [DepthStabilizer (optional)] → OpenXR Runtime
```

DepthStabilizer maintains per-stream state:
- Previous frame buffer (640x480x2 bytes)
- Configuration (alpha, delta parameters)
- Enable/disable flag for performance-critical applications

Processing flow:
1. Receive raw depth frame from libfreenect
2. For each pixel: apply exponential moving average if within delta threshold
3. Handle invalid pixels (FREENECT_DEPTH_MM_NO_VALUE) by bypassing filter
4. Output stabilized frame to OpenXR

Design principles:
- Single-pass processing (<1ms target)
- Stateless operation (can enable/disable per-frame)
- Separate state per stream (switching sources doesn't pollute history)
- Configurable smoothing strength vs. latency tradeoff

## Milestones

**Research milestones (all complete):**

- [x] Investigate libfreenect built-in filtering capabilities
- [x] Research temporal filtering techniques (median, exponential moving average)
- [x] Research spatial filtering techniques (bilateral, joint bilateral, hybrid)
- [x] Analyze real-time performance constraints for 30 FPS operation
- [x] Document best practices from academic literature and industry implementations
- [x] Select recommended approach with implementation outline
- [x] Generate comprehensive research output (YAML + spec)

**Implementation milestones (if pursued in future spec):**

- [ ] Create DepthStabilizer class with configurable alpha/delta parameters
- [ ] Implement exponential moving average filtering algorithm
- [ ] Add invalid pixel handling (FREENECT_DEPTH_MM_NO_VALUE bypass)
- [ ] Integrate into KinectDevice depth callback pipeline
- [ ] Add configuration API (enable/disable, set alpha/delta)
- [ ] Performance validation (<1ms processing time at 640x480)
- [ ] Integration tests comparing filtered vs. raw depth streams

## Open Questions

**Research complete - No open questions for this phase.**

Questions deferred to implementation phase (if pursued):
- Should DepthStabilizer support both depth and IR streams, or depth only?
- Should filter parameters be exposed via OpenXR vendor extension or internal API?
- What is the optimal default alpha value (0.4 recommended, needs empirical validation)?
- Should we provide multiple presets (low/medium/high smoothing) or direct parameter control?

---

## Implementation Log

### Research Phase (2026-02-06)

Conducted comprehensive literature review and industry analysis:

**libfreenect Analysis:**
- Examined `/opt/homebrew/include/libfreenect/libfreenect.h`
- Confirmed: No built-in filtering (raw depth only)
- Note: libfreenect2 has bilateral/edge-aware filters but targets Kinect v2 hardware

**Web Research:**
- Searched 6 distinct queries covering temporal filtering, bilateral filtering, Kinect-specific techniques
- Key finding: Intel RealSense temporal filter achieves <1ms processing with single-frame memory
- Key finding: Temporal median filter effective but introduces 66-133ms latency (unacceptable for XR)
- Key finding: Bilateral filtering requires GPU acceleration for real-time performance

**Sources Reviewed:**
1. IEEE papers on Kinect depth filtering and temporal stabilization
2. Intel RealSense post-processing filter documentation and implementation
3. libfreenect2 depth packet processor source code (OpenGL/OpenCL implementations)
4. Academic papers on bilateral filtering and joint bilateral filtering
5. Industry best practices from depth camera manufacturers

**Decision Rationale:**
Selected temporal exponential moving average over median filter despite median filter's superior noise reduction because:
- XR applications require <66ms latency (median needs 66-133ms)
- Intel RealSense validates <1ms processing time is achievable
- Configurable alpha parameter allows tuning smoothing vs. responsiveness
- Single-frame memory footprint more practical than 5-9 frame buffer

**Dead Ends:**
- GPU acceleration research deferred (out of scope for Phase 1)
- Joint bilateral filtering too complex for initial implementation (requires color-depth alignment)
- Hybrid pipelines (RealSense-style) deferred pending Phase 1 validation

---

## Documentation Updates

**Research-only spec - No implementation performed.**

### Future Documentation (if implementation proceeds):

**docs/ARCHITECTURE.md:**
- Add DepthStabilizer component to Phase 1 architecture diagram
- Document depth processing pipeline: libfreenect → DepthStabilizer → OpenXR
- Describe exponential moving average algorithm and performance characteristics
- Note configuration options (alpha, delta, enable/disable)

**docs/PRD.md:**
- Optional depth stabilization feature (user-configurable)
- Tradeoff: Improved depth quality vs. slight latency increase
- Use case: Static or slow-moving XR scenes benefit most

**README.md:**
- Document depth stabilization configuration options
- Guidance on when to enable/disable (static vs. dynamic scenes)

---

## Research Sources

### Academic Literature
1. [Temporal filtering for depth maps generated by Kinect depth camera (IEEE)](https://ieeexplore.ieee.org/document/5877202/)
2. [Characterizations of Noise in Kinect Depth Images: A Review (ResearchGate)](https://www.researchgate.net/publication/261601243_Characterizations_of_Noise_in_Kinect_Depth_Images_A_Review)
3. [DENOISING TIME-OF-FLIGHT DEPTH MAPS USING TEMPORAL MEDIAN FILTER (Semantic Scholar)](https://www.semanticscholar.org/paper/DENOISING-TIME-OF-FLIGHT-DEPTH-MAPS-USING-TEMPORAL-Lin-Wu/83e6896229858a927dbcd98aabb8352b58faeb21)
4. [Methods for depth-map filtering in view-plus-depth 3D video (EURASIP)](https://asp-eurasipjournals.springeropen.com/articles/10.1186/1687-6180-2012-25)
5. [Directional Joint Bilateral Filter for Depth Images (MDPI)](https://www.mdpi.com/1424-8220/14/7/11362)
6. [A Gentle Introduction to Bilateral Filtering (MIT CSAIL)](https://people.csail.mit.edu/sparis/bf_course/course_notes.pdf)

### Industry Implementations
7. [Intel RealSense Post-processing Filters (Documentation)](https://dev.intelrealsense.com/docs/post-processing-filters)
8. [librealsense Post-processing Filters (GitHub)](https://github.com/IntelRealSense/librealsense/blob/master/doc/post-processing-filters.md)
9. [Intel RealSense Temporal Filter Implementation (GitHub)](https://github.com/IntelRealSense/librealsense/blob/master/src/proc/temporal-filter.cpp)
10. [Depth Post-Processing for Intel RealSense D400 Series (Mouser)](https://www.mouser.com/pdfdocs/Intel-RealSense-Depth-PostProcess.pdf)

### OpenKinect Community
11. [Smoothing Kinect Depth Frames in Real-Time (CodeProject)](https://www.codeproject.com/Articles/317974/KinectDepthSmoothing)
12. [libfreenect2 Depth Packet Processor Implementation (GitHub)](https://github.com/OpenKinect/libfreenect2/issues/134)
13. [Kinect Smoothing Repository (GitHub)](https://github.com/intelligent-control-lab/Kinect_Smoothing)

---

## Archive Criteria

**Research-only spec - All criteria met:**

- [x] All milestones complete (research phase)
- [x] All acceptance criteria met
- [x] Tests passing (N/A - no implementation)
- [x] **Proposed doc updates drafted** in section above
- [x] **PRD.md updates identified** (optional stabilization feature)
- [x] **ARCHITECTURE.md updates identified** (DepthStabilizer component)
- [x] **Status set to complete** (research finished)
- [ ] Spec moved to `specs/archive/012-DepthStabilizationModule.md` (manual step)
