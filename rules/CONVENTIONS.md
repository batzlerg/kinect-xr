# C++ Conventions - Kinect XR

## Style

### Formatter
- **Tool:** clang-format (configured in `.clang-format`)
- **Auto-format on save:** Configured in `.vscode/settings.json`

### Language Standard
- **C++17** (required, no extensions)
- Set via CMake: `CMAKE_CXX_STANDARD 17`

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `KinectDevice` |
| Functions | camelCase | `initialize()`, `getDeviceCount()` |
| Variables | camelCase | `deviceCount`, `config` |
| Private members | trailing underscore | `ctx_`, `dev_`, `initialized_` |
| Constants | ALL_CAPS | `MAX_DEVICES` |
| Enums | PascalCase (enum class) | `DeviceError::None` |
| Namespaces | snake_case | `kinect_xr` |
| Macros | ALL_CAPS | `KINECT_XR_VERSION` |

### File Naming

| Type | Convention | Example |
|------|------------|---------|
| Headers | snake_case.h | `device.h` |
| Implementation | snake_case.cpp | `device.cpp` |
| Test files | snake_case_test.cpp | `device_unit_test.cpp` |

## Code Patterns

### Resource Management (RAII)

All resources managed via constructors/destructors:

```cpp
class KinectDevice {
public:
  KinectDevice();
  ~KinectDevice();

  // Prevent accidental copies (one instance per physical device)
  KinectDevice(const KinectDevice&) = delete;
  KinectDevice& operator=(const KinectDevice&) = delete;

private:
  freenect_context* ctx_;  // Cleaned up in destructor
  freenect_device* dev_;
};
```

### Error Handling

Use `DeviceError` enum (not exceptions):

```cpp
// Good: Return error code
DeviceError result = device.initialize(config);
if (result != DeviceError::None) {
  std::cerr << errorToString(result) << std::endl;
}

// Avoid: Exceptions
try { device.initialize(config); } catch (...) { }
```

### Forward Declarations

Minimize includes in headers:

```cpp
// device.h - forward declare instead of including
struct _freenect_context;
typedef struct _freenect_context freenect_context;
```

### Const Correctness

```cpp
// Const member functions for read-only operations
bool isInitialized() const;
const DeviceConfig& getConfig() const;

// Const references for input parameters
DeviceError initialize(const DeviceConfig& config);
```

### Include Order

1. Corresponding header (for .cpp files)
2. C standard library
3. C++ standard library
4. External libraries (libfreenect)
5. Project headers

```cpp
#include "kinect_xr/device.h"  // Corresponding header first

#include <cstdint>              // C standard
#include <iostream>             // C++ standard
#include <memory>

#include <libfreenect/libfreenect.h>  // External

#include "kinect_xr/other.h"   // Project headers
```

## Documentation

### When to Comment

- **Complex algorithms:** Explain the "why"
- **Non-obvious decisions:** Document rationale
- **Public API:** Use Doxygen-style comments

```cpp
/**
 * @brief Initialize the Kinect device
 * @param config Configuration settings
 * @return DeviceError Error code, None if successful
 */
DeviceError initialize(const DeviceConfig& config);
```

### When NOT to Comment

- Self-explanatory code
- Implementation details that may change
- Obvious getter/setter methods

## Build System

### CMake Targets

| Target | Purpose |
|--------|---------|
| `kinect_xr_runtime` | Main executable |
| `kinect_xr_device` | Device abstraction library |
| `unit_tests` | Unit test executable |
| `integration_tests` | Integration test executable |

### Adding New Source Files

1. Add to appropriate `CMakeLists.txt`
2. Follow existing target patterns
3. Link required dependencies

## Dependencies

### Adding External Dependencies

Prefer this order:
1. **System-provided** (`find_library`) - for stable, common libraries
2. **FetchContent** - for build-time bundling (like GoogleTest)
3. **Submodules** - last resort

Document rationale in `CMakeLists.txt` comments.
