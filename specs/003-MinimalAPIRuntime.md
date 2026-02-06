# Minimal API Runtime

**Status:** draft
**Created:** 2026-02-05
**Branch:** feature/003-MinimalAPIRuntime
**Blocked By:** 001-StreamManagement, 002-OpenXRSDKSpike

## Summary

Implement the smallest possible OpenXR runtime that can be discovered and enumerated by OpenXR applications. This validates the runtime registration mechanism and OpenXR API integration before tackling graphics/swapchain complexity.

**What:** Implement `xrCreateInstance`, `xrDestroyInstance`, `xrGetInstanceProcAddr`, and `xrEnumerateInstanceExtensionProperties`. Create runtime manifest for macOS registration. Verify discoverability via OpenXR Explorer or similar tool.

**Why:** The "Minimal API Runtime" approach reduces risk by validating the smallest integration surface first. If runtime registration or API negotiation fails, we discover this before investing in Metal/swapchain implementation. This is a clear checkpoint before Phase 3.

**Current State:**
- Phase 1 device layer complete (after Spec 001)
- OpenXR SDK integrated (after Spec 002)
- No OpenXR runtime implementation exists

**Technical Approach:**
1. Create runtime library implementing OpenXR entry points
2. Use GetInstanceProcAddr pattern for API negotiation
3. Create JSON manifest file for runtime registration
4. Register runtime via XR_RUNTIME_JSON environment variable (development)
5. Test with OpenXR Explorer or simple enumeration app

**Testing Strategy:**
- Unit tests verify instance lifecycle without graphics
- Integration test uses XR_RUNTIME_JSON to select our runtime
- Validation via OpenXR Explorer (external tool)
- No hardware required for this spec

## Scope

**In:**
- Implement xrGetInstanceProcAddr (runtime entry point)
- Implement xrEnumerateInstanceExtensionProperties (return XR_KHR_composition_layer_depth)
- Implement xrCreateInstance / xrDestroyInstance
- Implement xrEnumerateApiLayerProperties (return empty list)
- Create KinectXRRuntime class to manage instance state
- Create runtime manifest JSON file
- Create installation/registration script for development
- Add unit tests for instance lifecycle
- Document runtime registration for macOS

**Out:**
- Session management (xrCreateSession, etc.)
- Swapchain implementation
- Metal/graphics integration
- Frame submission
- Reference spaces
- Action system
- Actual XR functionality beyond discovery

## Acceptance Criteria

**Does this spec involve UI-observable behavior?** NO (library, no UI)

### Descriptive Criteria

- [ ] Runtime library builds successfully
- [ ] xrEnumerateInstanceExtensionProperties returns XR_KHR_composition_layer_depth
- [ ] xrCreateInstance succeeds and returns valid handle
- [ ] xrDestroyInstance cleans up resources
- [ ] Runtime manifest JSON is valid
- [ ] Runtime discoverable via XR_RUNTIME_JSON environment variable
- [ ] OpenXR Explorer (or equivalent) can enumerate our runtime
- [ ] Unit tests pass for instance lifecycle
- [ ] No memory leaks in instance create/destroy cycle

## Architecture Delta

**Before:**
kinect-xr has device layer only. No OpenXR integration.

```
Application → (nothing) → KinectDevice → libfreenect → Kinect
```

**After:**
kinect-xr has minimal OpenXR runtime layer. Applications can discover and create instances, but cannot create sessions or render.

```
Application → OpenXR Loader → KinectXRRuntime → (future: KinectDevice)
                    ↑
            Runtime Manifest (JSON)
```

**New Components:**
- `src/runtime/` - OpenXR runtime implementation
- `src/runtime/kinect_xr_runtime.cpp` - KinectXRRuntime class
- `src/runtime/entry_points.cpp` - OpenXR function implementations
- `include/kinect_xr/runtime.h` - Runtime public interface
- `manifest/kinect_xr.json` - Runtime manifest

## Milestones

- [ ] **M1: Create runtime library structure**
  - Create src/runtime/ directory
  - Create include/kinect_xr/runtime.h with KinectXRRuntime class declaration
  - Add CMake target kinect_xr_runtime (shared library)
  - Link against OpenXR loader headers
  - Validation: Empty library builds

- [ ] **M2: Implement xrGetInstanceProcAddr**
  - Create entry_points.cpp with xrGetInstanceProcAddr implementation
  - Return function pointers for supported functions
  - Return nullptr for unsupported functions
  - Export as C symbol (extern "C")
  - Validation: Function compiles, returns valid pointers

- [ ] **M3: Implement instance enumeration functions**
  - Implement xrEnumerateInstanceExtensionProperties:
    - Return XR_KHR_composition_layer_depth extension
    - Handle two-call idiom (count query, then data query)
  - Implement xrEnumerateApiLayerProperties:
    - Return empty list (no layers)
  - Validation: Unit test verifies extension enumeration

- [ ] **M4: Implement instance lifecycle**
  - Create KinectXRRuntime class to manage instance state
  - Implement xrCreateInstance:
    - Validate XrInstanceCreateInfo
    - Check requested extensions are supported
    - Create and return XrInstance handle
    - Store instance in runtime singleton
  - Implement xrDestroyInstance:
    - Validate handle
    - Clean up instance resources
    - Remove from runtime singleton
  - Validation: Unit test creates and destroys instance

- [ ] **M5: Create runtime manifest**
  - Create manifest/kinect_xr.json:
    ```json
    {
        "file_format_version": "1.0.0",
        "runtime": {
            "name": "Kinect XR Runtime",
            "library_path": "./libkinect_xr_runtime.dylib"
        }
    }
    ```
  - Create scripts/register-runtime.sh for development:
    ```bash
    export XR_RUNTIME_JSON=/path/to/kinect_xr.json
    ```
  - Document registration in CLAUDE.md
  - Validation: Manifest parses correctly

- [ ] **M6: Integration test with OpenXR loader**
  - Create tests/integration/runtime_discovery_test.cpp
  - Test sets XR_RUNTIME_JSON to our manifest
  - Test calls xrEnumerateInstanceExtensionProperties via loader
  - Test creates and destroys instance via loader
  - Verify our runtime is selected
  - Validation: Integration test passes

- [ ] **M7: Validate with external tool**
  - Install OpenXR Explorer or similar validation tool
  - Set XR_RUNTIME_JSON to our manifest
  - Launch tool and verify runtime appears
  - Verify extension enumeration shows our extensions
  - Document validation results
  - Validation: External tool confirms runtime discovery

## Open Questions

**Q1: Should runtime be static or shared library?**
- OpenXR loaders typically expect shared libraries (.dylib on macOS)
- Recommendation: Shared library (required for runtime discovery)
- Decision: Shared library

**Q2: How to handle XrInstance internally?**
- Option A: Use sequential integers cast to XrInstance
- Option B: Use pointer to internal InstanceData struct
- Recommendation: Option B - safer, allows validation
- Decision: TBD during M4

**Q3: Should we implement xrGetInstanceProperties?**
- Technically optional for minimal runtime
- Useful for identification in tools
- Recommendation: Yes, implement minimal version
- Decision: Add to M4 if time permits

---

## Implementation Log

### Milestone 1
-

### Milestone 2
-

### Milestone 3
-

### Milestone 4
-

### Milestone 5
-

### Milestone 6
-

### Milestone 7
-

---

## Documentation Updates

### CLAUDE.md

Add section:
```markdown
## OpenXR Runtime Development

To use the Kinect XR runtime during development:

\`\`\`bash
export XR_RUNTIME_JSON=/path/to/kinect-xr/manifest/kinect_xr.json
\`\`\`

Then run any OpenXR application - it will use our runtime.
```

### docs/ARCHITECTURE.md

Add new section after "Device Abstraction Layer":

```markdown
### OpenXR Runtime Layer (Phase 2)

**Purpose:** Standards-compliant OpenXR API implementation.

**Files:**
- `src/runtime/kinect_xr_runtime.cpp` - Runtime singleton
- `src/runtime/entry_points.cpp` - OpenXR function implementations
- `include/kinect_xr/runtime.h` - Public interface
- `manifest/kinect_xr.json` - Runtime manifest

**Supported Extensions:**
- XR_KHR_composition_layer_depth

**Registration:**
Runtime discovered via XR_RUNTIME_JSON environment variable.
```

### docs/PRD.md

Update Phase 2 success criteria:
- [ ] xrEnumerateInstanceExtensionProperties returns our extensions
- [ ] xrCreateInstance/xrDestroyInstance work correctly
- [ ] Runtime discoverable via OpenXR Explorer

---

## Archive Criteria

- [ ] All milestones complete
- [ ] All acceptance criteria met
- [ ] Tests passing
- [ ] Runtime discoverable by external tool
- [ ] Documentation updated
- [ ] Spec moved to specs/archive/003-MinimalAPIRuntime.md
