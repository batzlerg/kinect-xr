# OpenXR SDK Integration

**Date:** 2026-02-05
**Spec:** 002-OpenXRSDKSpike.md
**Status:** Complete

## Summary

Successfully integrated OpenXR SDK 1.1.54 into the kinect-xr build system on macOS (Apple Silicon). The SDK compiles and links correctly using CMake's FetchContent, and the spike test validates that the OpenXR loader is functional.

## OpenXR SDK Version

**Version:** 1.1.54 (release-1.1.54)
**Source:** https://github.com/KhronosGroup/OpenXR-SDK-Source.git
**Release Date:** 2025-12-02

This is the latest stable release as of February 2026.

## CMake Configuration

### Integration Approach

Added OpenXR SDK using CMake's FetchContent mechanism, following the same pattern used for GoogleTest:

```cmake
# OpenXR SDK
FetchContent_Declare(
  OpenXR
  GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK-Source.git
  GIT_TAG release-1.1.54
)
FetchContent_MakeAvailable(OpenXR)
```

### Targets Available

The OpenXR SDK provides the following CMake targets:
- `openxr_loader` - The main OpenXR loader library (links applications to runtimes)
- `XrApiLayer_api_dump` - API call debugging layer
- `XrApiLayer_core_validation` - Validation layer for API calls
- `XrApiLayer_best_practices_validation` - Best practices validation layer
- `hello_xr` - Example OpenXR application (built by SDK)

For our runtime implementation, we will primarily link against `openxr_loader`.

## Build Behavior

### First Build (Clean)

- **FetchContent clone time:** ~5-10 seconds
- **OpenXR build time:** ~30-40 seconds (includes loader, layers, Catch2, hello_xr)
- **Total additional build time:** ~45-50 seconds on first build
- **Subsequent builds:** Incremental, minimal overhead

### Build Artifacts

The SDK generates:
- `build/lib/libopenxr_loader.dylib` - OpenXR loader shared library
- `build/_deps/openxr-build/include/openxr/` - Generated OpenXR headers
- Several API layer shared modules (.so files)
- Test executables (loader_test, hello_xr, etc.)

## Warnings Encountered

### 1. Metal Tools Not Found

```
-- Could NOT find MetalTools (missing: MetalTools_METALLIB_EXECUTABLE)
-- Metal not found - full Xcode install required
xcrun: error: unable to find utility "metallib", not a developer tool or in PATH
```

**Impact:** None for our project. Metal backend is optional in OpenXR SDK. We don't need Metal for the loader or for Phase 2 runtime implementation.

**Resolution:** No action required. This is expected on systems without full Xcode installation.

### 2. Deprecated `getenv` Warning

Multiple warnings during loader compilation:
```
warning: Warning: Falling back to non-secure getenv for environmental lookups!
Consider updating to a different libc. [-W#pragma-messages]
```

**Impact:** None. This is an OpenXR SDK internal warning about using `getenv` instead of `secure_getenv`. macOS doesn't provide `secure_getenv`.

**Resolution:** No action required. This is a known limitation on macOS and doesn't affect functionality.

### 3. Linker Warning

```
ld: warning: -undefined error is deprecated
```

**Impact:** None. This is a linker flag deprecation warning from the OpenXR SDK's CMake configuration.

**Resolution:** No action required. Upstream SDK issue, does not affect our build.

## Spike Test Results

### Test Implementation

Created minimal test at `tests/spike/openxr_sdk_test.cpp`:
- Includes `<openxr/openxr.h>`
- Calls `xrEnumerateInstanceExtensionProperties()`
- Prints result code and extension count

### Runtime Behavior

```
OpenXR SDK linked successfully
xrEnumerateInstanceExtensionProperties returned: -51
Extension count: 0
Error [GENERAL | xrEnumerateInstanceExtensionProperties | OpenXR-Loader] : RuntimeManifestFile::FindManifestFiles - failed to determine active runtime file path for this environment
Error [GENERAL | xrEnumerateInstanceExtensionProperties | OpenXR-Loader] : RuntimeInterface::LoadRuntimes - unknown error
Error [GENERAL | xrEnumerateInstanceExtensionProperties | OpenXR-Loader] : RuntimeInterface::LoadRuntimes - failed to load a runtime
Error [GENERAL | xrEnumerateInstanceExtensionProperties | OpenXR-Loader] : Failed to find default runtime with RuntimeInterface::LoadRuntime()
Error [GENERAL | xrEnumerateInstanceExtensionProperties | OpenXR-Loader] : Failed querying extension properties
```

**Result Code:** -51 (`XR_ERROR_RUNTIME_UNAVAILABLE`)

**Analysis:** This is the **expected and correct behavior**. The error indicates:
1. The OpenXR loader is functioning properly
2. Headers are included correctly
3. Linking is successful
4. The loader attempts to find an OpenXR runtime manifest but (correctly) finds none installed

This validates that the SDK integration is working. Once we implement our runtime in Phase 2 and register it via a JSON manifest, this error will resolve.

## macOS-Specific Issues

### Filesystem Issues (Resolved)

The OpenXR SDK performed several filesystem capability tests during configuration:
```
-- Performing Test HAVE_FILESYSTEM_IN_STD - Failed
-- Performing Test HAVE_FILESYSTEM_IN_STDEXPERIMENTAL - Failed
-- Performing Test HAVE_FILESYSTEM_WITHOUT_LIB - Failed
```

**Resolution:** The SDK fell back to compatible alternatives. No impact on our build.

### No Platform-Specific Blockers

All tests passed. The OpenXR SDK builds cleanly on macOS with Apple Silicon (arm64).

## Recommendations for Phase 2

### 1. Link Against openxr_loader

When implementing the runtime (Phase 2):
```cmake
target_link_libraries(kinect_xr_runtime
  PRIVATE
  openxr_loader
  ${LIBFREENECT_LIBRARY}
)
```

### 2. Runtime Manifest Registration

The runtime will need to register itself via a JSON manifest file. The loader searches for runtimes in platform-specific locations:

**macOS:**
- User: `~/.config/openxr/1/active_runtime.json`
- System: `/usr/local/share/openxr/1/active_runtime.json`

The manifest should look like:
```json
{
  "file_format_version": "1.0.0",
  "runtime": {
    "library_path": "/path/to/libkinect_xr_runtime.dylib",
    "name": "Kinect XR Runtime"
  }
}
```

### 3. Enable Validation Layers During Development

The SDK includes validation layers that can catch API misuse:
- Set `XR_ENABLE_API_LAYERS=XR_APILAYER_LUNARG_core_validation` environment variable
- Validation output will help debug runtime implementation

### 4. Reference Implementation

The SDK includes `hello_xr` sample application:
- Built at `build/bin/hello_xr`
- Good reference for minimal OpenXR application structure
- Can be used to test our runtime once implemented

### 5. Header Locations

OpenXR headers are available at:
- `build/_deps/openxr-build/include/openxr/openxr.h`
- Auto-included via `openxr_loader` target, no manual include paths needed

## Next Steps

1. **Spec 001 (Device Architecture)** - Complete in parallel
2. **Phase 2 Implementation** - Use `openxr_loader` target, implement minimal runtime API
3. **Manifest Setup** - Create JSON manifest for runtime registration
4. **Integration Testing** - Use `hello_xr` to validate runtime

## Appendix: Build Targets

Full list of targets added by OpenXR SDK:

```
openxr_loader                          - Main loader library
XrApiLayer_api_dump                   - API debugging layer
XrApiLayer_core_validation            - Core validation layer
XrApiLayer_best_practices_validation  - Best practices layer
hello_xr                              - Example application
loader_test                           - Loader unit tests
openxr_runtime_list                   - Runtime enumeration tool
openxr_runtime_list_json              - Runtime JSON tool
test_runtime                          - Test runtime stub
XrApiLayer_test                       - Test layer
Catch2                                - Test framework (bundled)
```

Most of these are for SDK development/testing and won't be needed for our runtime implementation.
