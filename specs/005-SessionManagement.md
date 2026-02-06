# Session Management

**Status:** draft
**Created:** 2026-02-05
**Branch:** main
**Blocked By:** None

## Summary

Implement OpenXR session lifecycle management to enable XR applications to create and manage sessions with the Kinect runtime. This bridges instance management (spec 003) and future rendering capabilities (spec 006+), establishing the Kinect as a discoverable XR system.

**What:** Implement system enumeration (`xrGetSystem`, `xrGetSystemProperties`), view configuration APIs (`xrEnumerateViewConfigurationTypes`, `xrEnumerateViewConfigurationViews`), session lifecycle (`xrCreateSession`, `xrBeginSession`, `xrEndSession`, `xrDestroySession`), and reference space management (`xrEnumerateReferenceSpaceTypes`, `xrCreateReferenceSpace`, `xrDestroySpace`).

**Why:** Session management is the foundation for all XR rendering. Applications must successfully create sessions before they can render frames or submit depth data. By implementing session APIs now, we can validate the runtime's core lifecycle with tools like `hello_xr` before tackling swapchain complexity in Phase 3.

**Current State:**
- Phase 1 (Device Integration): Complete - streaming works
- Spec 003 (Minimal API Runtime): Complete - instance discovery works
- Spec 004 (Metal Spike): Complete - texture upload patterns validated
- Session APIs: Not implemented - applications cannot create sessions

**Technical Approach:**
1. Extend `KinectXRRuntime` with session and system state management
2. Represent Kinect as `XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY` (closest semantic match for camera-on-world view)
3. Use mono view configuration (`XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO`) with single 640x480 view
4. Implement simplified session state machine (IDLE → READY → SYNCHRONIZED → VISIBLE → FOCUSED)
5. Support Metal graphics binding (`XR_KHR_metal_enable` extension)
6. Provide LOCAL, VIEW, and STAGE reference spaces

**Testing Strategy:**
- Unit tests verify session lifecycle without graphics binding
- Integration tests validate with Metal graphics binding
- External validation with `hello_xr -g Vulkan` (will fail at swapchain, but should succeed at session creation)
- No Kinect hardware required for these tests (session management is pure OpenXR state)

## Scope

**In:**
- System enumeration and properties
- View configuration enumeration
- Session lifecycle (create/begin/end/destroy)
- Session state management
- Event queue for state changes
- Reference spaces (VIEW, LOCAL, STAGE)
- Graphics requirements (Metal)
- `XR_KHR_metal_enable` extension support
- SessionData structure for per-session state
- Handle validation for systems, sessions, and spaces

**Out:**
- Swapchain management - Spec 006
- Frame rendering (xrWaitFrame, xrBeginFrame, xrEndFrame) - Spec 006
- Action system - Deferred to Phase 6
- View pose tracking - Spec 007
- Motor control - Deferred to Phase 6
- Multi-session support (only one session per instance)
- Advanced reference spaces (UNBOUNDED, LOCAL_FLOOR)
- Actual rendering or device streaming integration

## Acceptance Criteria

**Does this spec involve UI-observable behavior?** NO (library, no UI)

### Descriptive Criteria

- [ ] `xrGetSystem` returns valid system ID for HEAD_MOUNTED_DISPLAY form factor
- [ ] `xrGetSystemProperties` reports Kinect as "Kinect XR System" with 640x480 max resolution
- [ ] `xrEnumerateViewConfigurationTypes` returns PRIMARY_MONO only
- [ ] `xrEnumerateViewConfigurationViews` returns single view (640x480 recommended/max)
- [ ] `xrCreateSession` succeeds with Metal graphics binding
- [ ] `xrCreateSession` validates graphics binding chain (fails without binding)
- [ ] `xrBeginSession` transitions session to SYNCHRONIZED state
- [ ] `xrEndSession` transitions session to STOPPING then IDLE state
- [ ] `xrDestroySession` cleans up resources (no leaks)
- [ ] `xrEnumerateReferenceSpaceTypes` returns VIEW, LOCAL, STAGE
- [ ] `xrCreateReferenceSpace` creates valid space handles for all supported types
- [ ] `xrDestroySpace` validates handles and cleans up
- [ ] `xrGetMetalGraphicsRequirementsKHR` returns valid Metal device requirements
- [ ] Invalid handles rejected with `XR_ERROR_HANDLE_INVALID`
- [ ] Session state events queued properly (`XrEventDataSessionStateChanged`)
- [ ] Unit tests pass (12+ tests covering lifecycle)
- [ ] Integration tests pass with Metal binding
- [ ] No memory leaks in create/destroy cycles
- [ ] `hello_xr` progresses further than before (creates session successfully)

## Architecture Delta

**Before:**
Runtime supports instance creation but cannot create sessions. Applications can discover runtime but cannot use it for XR.

**After:**
Runtime supports full session lifecycle. Applications can create sessions with Metal binding, enumerate views, and create reference spaces. Ready for swapchain implementation.

**New Components:**
- SessionData, SystemData structures in runtime.h
- Session/system/space management in KinectXRRuntime
- Event queue for state changes
- 15+ new OpenXR entry points

## Milestones

- [x] **M1: System enumeration and properties**
  - Add SystemData struct
  - Implement xrGetSystem
  - Implement xrGetSystemProperties
  - Add to xrGetInstanceProcAddr
  - Validation: Unit tests pass

- [ ] **M2: View configuration enumeration**
  - Implement xrEnumerateViewConfigurationTypes
  - Implement xrGetViewConfigurationProperties
  - Implement xrEnumerateViewConfigurationViews
  - Validation: Returns PRIMARY_MONO, 640x480 view

- [ ] **M3: Session creation and destruction**
  - Add SessionData struct
  - Implement xrCreateSession (validate Metal binding)
  - Implement xrDestroySession
  - Add to xrGetInstanceProcAddr
  - Validation: Create/destroy works

- [ ] **M4: Session state machine and begin/end**
  - Implement state machine in SessionData
  - Implement event queue
  - Implement xrPollEvent
  - Implement xrBeginSession
  - Implement xrEndSession
  - Validation: State transitions correct

- [ ] **M5: Reference space management**
  - Add SpaceData struct
  - Implement xrEnumerateReferenceSpaceTypes
  - Implement xrCreateReferenceSpace
  - Implement xrDestroySpace
  - Validation: All space types work

- [ ] **M6: Graphics requirements**
  - Implement xrGetMetalGraphicsRequirementsKHR
  - Add to xrGetInstanceProcAddr
  - Validation: Returns valid requirements

- [ ] **M7: Integration testing and external validation**
  - Create session_lifecycle_test.cpp
  - Run hello_xr and document progress
  - Verify no memory leaks
  - Validation: All tests pass, hello_xr progresses

## Open Questions

**Q1: Should Kinect be HEAD_MOUNTED_DISPLAY or HANDHELD_DISPLAY?**
- **Recommendation:** HEAD_MOUNTED_DISPLAY - Standard form factor for XR runtimes
- **Decision:** HEAD_MOUNTED_DISPLAY

**Q2: Full session state machine or simplified?**
- **Recommendation:** Simplified (skip LOSS_PENDING/EXITING)
- **Decision:** Simplified

**Q3: Origin of LOCAL and STAGE reference spaces?**
- **Recommendation:** All spaces at camera origin (identity transforms)
- **Decision:** Identity transforms, defer calibration to spec 007

**Q4: Multiple simultaneous sessions?**
- **Recommendation:** No - Single session per instance
- **Decision:** Single session only

**Q5: Metal graphics binding validation?**
- **Recommendation:** Store pointers, validate in spec 006
- **Decision:** No Metal API calls in this spec

## Implementation Log

### Milestone 1
- Added SystemData struct to runtime.h with systemId and formFactor
- Implemented xrGetSystem - creates system lazily on first call, validates HEAD_MOUNTED_DISPLAY form factor
- Implemented xrGetSystemProperties - returns Kinect specifications (640x480, no tracking)
- Added isValidSystem() helper for system ID validation
- Created system_management_test.cpp with 8 unit tests
- All tests passing - system enumeration working correctly

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

## Documentation Updates

### CLAUDE.md
Add section on session lifecycle with code examples.

### docs/ARCHITECTURE.md
Update OpenXR Runtime Layer section with session management details.

### docs/PRD.md
Update Phase 2 success criteria checkboxes.

## Archive Criteria

- [ ] All milestones complete
- [ ] All acceptance criteria met
- [ ] All unit tests passing (12+ tests)
- [ ] Integration tests passing
- [ ] No memory leaks (verified with sanitizers)
- [ ] `hello_xr` validation documented
- [ ] Documentation updated (CLAUDE.md, ARCHITECTURE.md, PRD.md)
- [ ] Spec moved to specs/archive/005-SessionManagement.md
