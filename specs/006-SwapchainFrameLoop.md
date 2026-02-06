# Swapchain + Frame Loop

**Status:** draft
**Created:** 2026-02-05
**Branch:** main
**Blocked By:** None

## Summary

Implement OpenXR swapchain management and frame rendering loop, bridging session management (Spec 005) with depth layer integration (Spec 007). The critical challenge is handling the **30Hz Kinect → 90Hz VR runtime frame rate mismatch** using triple buffering and frame repetition.

**What:** Implement OpenXR swapchain APIs (`xrCreateSwapchain`, `xrAcquireSwapchainImage`, `xrReleaseSwapchainImage`) backed by Metal textures, plus frame loop timing APIs (`xrWaitFrame`, `xrBeginFrame`, `xrEndFrame`), and view pose queries (`xrLocateViews`).

**Why:** Applications cannot render XR content without swapchains. This is the foundation for visual output and enables hello_xr to complete its full frame loop.

**Current State:**
- Sessions work (Spec 005 complete)
- Metal patterns validated (Spec 004 complete)
- No swapchain implementation
- hello_xr fails at swapchain creation

**Technical Approach:**
1. Swapchain Management: Triple-buffered Metal texture swapchains (BGRA8Unorm)
2. Frame Rate Strategy: Cache latest Kinect frame (30Hz), repeat for VR runtime (60-90Hz)
3. Frame Loop: xrWaitFrame blocking at 30Hz, begin/end state machine
4. View Poses: Static camera pose (Kinect doesn't move)
5. Thread Safety: Mutex protection for frame cache

**Testing Strategy:**
- Unit tests: Frame timing logic without Metal/hardware
- Integration tests: Real Kinect + Metal swapchain rendering
- External validation: hello_xr should render frames

## Scope

**In:**
- xrEnumerateSwapchainFormats (BGRA8Unorm only)
- xrCreateSwapchain (3 Metal textures)
- xrDestroySwapchain
- xrEnumerateSwapchainImages (Metal texture handles)
- xrAcquireSwapchainImage (get next image)
- xrWaitSwapchainImage (always ready)
- xrReleaseSwapchainImage (return to pool)
- xrWaitFrame (30Hz pacing)
- xrBeginFrame (start rendering)
- xrEndFrame (submit layers)
- xrLocateViews (static camera pose)
- SwapchainData structure with Metal textures
- Frame timing state machine
- Kinect frame caching for repetition

**Out:**
- Depth layer submission (Spec 007)
- Actual Kinect streaming integration (Spec 007)
- Kinect-to-texture upload (Spec 007)
- Multi-swapchain support
- Advanced view configurations (stereo)
- Performance optimizations

## Acceptance Criteria

**Does this spec involve UI-observable behavior?** YES - hello_xr should render frames

### Descriptive Criteria

- [ ] xrEnumerateSwapchainFormats returns BGRA8Unorm
- [ ] xrCreateSwapchain creates 3 Metal textures
- [ ] xrEnumerateSwapchainImages returns 3 texture handles
- [ ] xrAcquireSwapchainImage cycles through images (0→1→2→0)
- [ ] xrWaitSwapchainImage succeeds immediately
- [ ] xrReleaseSwapchainImage validates image index
- [ ] xrWaitFrame blocks at 30Hz (~33.3ms intervals)
- [ ] xrBeginFrame validates session state
- [ ] xrEndFrame accepts color composition layers
- [ ] xrLocateViews returns static pose
- [ ] Swapchain handle validation works
- [ ] Thread safety (no race conditions)
- [ ] Unit tests pass (15+ tests, no hardware)
- [ ] Integration tests pass (Metal + hardware)
- [ ] hello_xr renders frames successfully

## Architecture Delta

**Before:** Runtime supports sessions but cannot create swapchains.

**After:** Runtime supports full frame rendering loop with Metal-backed swapchains.

**New Components:**
- SwapchainData structure
- FrameState structure
- Frame timing logic
- Metal texture swapchains

## Milestones

- [ ] **M1: Swapchain Data Structures** (unit tests only)
  - Add SwapchainData and FrameState structs
  - Add swapchain tracking to runtime
  - Unit tests for handle validation

- [ ] **M2: Format Enumeration** (unit tests)
  - Implement xrEnumerateSwapchainFormats
  - Return BGRA8Unorm
  - Unit test validates format

- [ ] **M3: Swapchain Lifecycle** (mock, unit tests)
  - Implement create/destroy/enumerate (no real Metal)
  - Unit tests for lifecycle
  - Use nullptr for Metal textures

- [ ] **M4: Metal Integration** (integration tests)
  - Create real MTLTexture objects
  - Store Metal device from session
  - Integration test validates Metal textures

- [ ] **M5: Image Acquire/Release** (unit + integration)
  - Implement acquire/wait/release
  - Image cycling (0→1→2→0)
  - Both unit and integration tests

- [ ] **M6: Frame Timing** (unit tests)
  - Implement xrWaitFrame (30Hz pacing)
  - Unit tests for timing intervals

- [ ] **M7: Frame State Machine** (unit tests)
  - Implement xrBeginFrame/xrEndFrame
  - State validation
  - Error condition tests

- [ ] **M8: View Pose Queries** (unit tests)
  - Implement xrLocateViews
  - Static camera pose
  - FOV validation

- [ ] **M9: External Validation** (integration)
  - Run hello_xr
  - Confirm frame rendering works
  - Integration test mimicking hello_xr

## Open Questions

**Q1: xrWaitFrame pacing?**
- **Decision:** Pace at 30Hz (Kinect native rate)

**Q2: Applications requesting >30Hz?**
- **Decision:** Accept request, but pace at 30Hz anyway

**Q3: Support >3 swapchain images?**
- **Decision:** 3 images only (triple buffering)

**Q4: Camera pose coordinate system?**
- **Decision:** Identity pose, OpenXR convention (-Z forward)

**Q5: Thread safety granularity?**
- **Decision:** Coarse mutex (sessionMutex_)

## Implementation Log

### Milestone 1
- (To be filled during implementation)

### Milestone 2
- (To be filled during implementation)

### Milestone 3
- (To be filled during implementation)

### Milestone 4
- (To be filled during implementation)

### Milestone 5
- (To be filled during implementation)

### Milestone 6
- (To be filled during implementation)

### Milestone 7
- (To be filled during implementation)

### Milestone 8
- (To be filled during implementation)

### Milestone 9
- (To be filled during implementation)

## Documentation Updates

### CLAUDE.md
Add section on swapchain development and frame loop patterns.

### docs/ARCHITECTURE.md
Update OpenXR Runtime Layer section with swapchain details.

### docs/PRD.md
Update Phase 3 success criteria checkboxes.

## Archive Criteria

- [ ] All milestones complete (M1-M9)
- [ ] All acceptance criteria met
- [ ] Unit tests passing (15+ tests)
- [ ] Integration tests passing (Metal + hardware)
- [ ] hello_xr validation documented
- [ ] No memory leaks (verified with sanitizers)
- [ ] Documentation updated
- [ ] Spec moved to specs/archive/006-SwapchainFrameLoop.md
