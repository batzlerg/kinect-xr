# OpenXR SDK Spike

**Status:** complete
**Created:** 2026-02-05
**Branch:** feature/002-OpenXRSDKSpike
**Blocked By:** None (can run in parallel with 001)

## Summary

Validate that OpenXR SDK can be integrated into the kinect-xr build system on macOS. This is a time-boxed spike to de-risk Phase 2 before full implementation begins.

**What:** Add OpenXR SDK to CMakeLists.txt via FetchContent, create minimal test that includes OpenXR headers and links against the loader, document any macOS-specific issues.

**Why:** Phase 2 (Minimal API Runtime) depends on OpenXR SDK integration. By validating the build system integration separately, we isolate build/toolchain issues from runtime implementation complexity. This follows the pattern already established with GoogleTest.

**Current State:**
- CMakeLists.txt uses FetchContent for GoogleTest successfully
- No OpenXR dependencies currently in project
- OpenXR-SDK-Source is the canonical open-source SDK

**Technical Approach:**
1. Add FetchContent_Declare for OpenXR-SDK-Source from Khronos GitHub
2. Link test executable against openxr_loader target
3. Create minimal test that calls xrEnumerateInstanceExtensionProperties
4. Verify build completes without errors on macOS (Apple Silicon)

**Testing Strategy:**
- Build success is the primary validation
- Runtime test can fail (no runtime installed) - we just need compilation and linking to work
- Document any compiler warnings or compatibility issues

## Scope

**In:**
- Add OpenXR SDK via FetchContent to CMakeLists.txt
- Create minimal test file (tests/spike/openxr_sdk_test.cpp)
- Verify OpenXR headers are includable
- Verify linking against openxr_loader succeeds
- Document macOS-specific configuration if needed
- Document any issues encountered

**Out:**
- Actual OpenXR runtime implementation
- Metal/graphics integration
- Runtime manifest creation
- Full test coverage
- Production-quality CMake configuration

## Acceptance Criteria

**Does this spec involve UI-observable behavior?** NO

### Descriptive Criteria

- [x] CMake configures successfully with OpenXR SDK
- [x] Test file includes `<openxr/openxr.h>` without errors
- [x] Test executable links against openxr_loader
- [x] Build completes on macOS (Apple Silicon)
- [x] Any issues or workarounds documented in docs/planning/

## Architecture Delta

**Before:**
No OpenXR dependencies. CMakeLists.txt only references libfreenect and GoogleTest.

**After:**
OpenXR SDK available via FetchContent. New spike test demonstrates header inclusion and loader linking. CMake configuration documented for future runtime implementation.

## Milestones

- [x] **M1: Add OpenXR SDK to CMakeLists.txt**
  - Add FetchContent_Declare for OpenXR-SDK-Source
  - Use appropriate git tag (latest stable, e.g., release-1.0.x)
  - Add FetchContent_MakeAvailable
  - Validation: cmake -B build -S . completes without errors

- [x] **M2: Create spike test file**
  - Create tests/spike/ directory
  - Create openxr_sdk_test.cpp with minimal OpenXR usage:
    ```cpp
    #include <openxr/openxr.h>
    #include <iostream>

    int main() {
        uint32_t extensionCount = 0;
        XrResult result = xrEnumerateInstanceExtensionProperties(
            nullptr, 0, &extensionCount, nullptr);

        std::cout << "OpenXR SDK linked successfully" << std::endl;
        std::cout << "xrEnumerateInstanceExtensionProperties returned: "
                  << result << std::endl;
        return 0;
    }
    ```
  - Add CMakeLists.txt in tests/spike/ to build test
  - Link against openxr_loader
  - Validation: Test compiles and links

- [x] **M3: Verify build and document findings**
  - Run full build: cmake --build build
  - Run spike test (expect XR_ERROR_RUNTIME_UNAVAILABLE - that's OK)
  - Document any warnings, workarounds, or macOS-specific issues
  - Create docs/planning/openxr-sdk-integration.md with findings
  - Validation: Documentation complete, build succeeds

## Open Questions

**Q1: Which OpenXR SDK version to use?**
- Recommendation: Use latest release tag (e.g., release-1.0.34)
- Check Khronos GitHub for current stable version
- Decision: Resolve during M1

**Q2: Should spike test be part of regular test suite?**
- Option A: Include in CTest, runs with other tests
- Option B: Separate spike directory, manual execution only
- Recommendation: Option B - spike is temporary validation, not ongoing test
- Decision: Option B

---

## Implementation Log

### Milestone 1
- Added OpenXR SDK 1.1.54 via FetchContent to CMakeLists.txt
- Used latest stable release tag: release-1.1.54 (2025-12-02)
- CMake configuration completed successfully
- Metal warnings are expected (not needed for loader)

### Milestone 2
- Created tests/spike/ directory structure
- Implemented openxr_sdk_test.cpp with xrEnumerateInstanceExtensionProperties call
- Created tests/spike/CMakeLists.txt linking against openxr_loader
- Added spike subdirectory to root CMakeLists.txt
- Full build completed successfully (100%)
- Spike test compiled and linked without errors

### Milestone 3
- Executed spike test: returned XR_ERROR_RUNTIME_UNAVAILABLE (-51) as expected
- No OpenXR runtime installed, so loader correctly reports unavailability
- This validates SDK integration is working correctly
- Documented all findings in docs/planning/openxr-sdk-integration.md
- Noted warnings: Metal tools not found (expected), getenv fallback (harmless)
- Build time impact: ~45-50 seconds first build, incremental afterward

---

## Documentation Updates

### docs/planning/openxr-sdk-integration.md (NEW FILE)

Create new file documenting:
- OpenXR SDK version used
- FetchContent configuration
- Any macOS-specific issues encountered
- Workarounds applied
- Build time impact
- Recommendations for Phase 2 implementation

---

## Archive Criteria

- [ ] All milestones complete
- [ ] All acceptance criteria met
- [ ] Findings documented in docs/planning/openxr-sdk-integration.md
- [ ] Spec moved to specs/archive/002-OpenXRSDKSpike.md
