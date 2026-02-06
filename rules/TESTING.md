# Testing Rules - Kinect XR

See `~/.claude/rules/TESTING.md` for universal testing principles.

## Framework

- **GoogleTest 1.13.0** (bundled via FetchContent)
- **CTest** for test discovery

## Running Tests

```bash
./test.sh                    # All tests

# Or manually:
cmake --build build
ctest --test-dir build --output-on-failure
```

## Test Categories

### Unit Tests (`tests/unit/`)

Hardware-independent tests that run anywhere:
- Error string conversion
- Construction/destruction
- Device enumeration (returns count, may be 0)
- Initialization behavior without hardware

**Run unit tests only:**
```bash
./build/bin/unit_tests
```

### Integration Tests (`tests/integration/`)

Require physical Kinect hardware:
- Device detection
- Device opening
- Stream start/stop
- Frame data validation

**Run integration tests only:**
```bash
./build/bin/integration_tests
```

**Skipping:** Integration tests use `GTEST_SKIP()` when no hardware:
```cpp
if (KinectDevice::getDeviceCount() == 0) {
  GTEST_SKIP() << "No Kinect device connected, skipping test";
}
```

## Test File Naming

- `*_unit_test.cpp` - Unit tests
- `*_integration_test.cpp` - Integration tests (hardware required)
- `*_test_main.cpp` - Test entry points

## TDD Pattern

This project uses TDD. Integration tests may define expected API before implementation:

```cpp
// Test written first (currently fails)
TEST_F(KinectDeviceTest, StartAndStopStreams) {
  EXPECT_FALSE(device->isStreaming());        // API not yet implemented
  EXPECT_EQ(device->startStreams(), DeviceError::None);
}
```

When tests define unimplemented API:
1. Implement the feature
2. Run tests to validate
3. Commit when green

## Adding New Tests

1. Create test file in appropriate directory (`unit/` or `integration/`)
2. Follow existing CMakeLists.txt patterns
3. Link against `kinect_xr_device` and `GTest::gtest_main`

## CI Considerations

Since integration tests require hardware:
- CI should run unit tests only
- Hardware testing done locally or with dedicated test runner
- Consider mocking libfreenect for CI (future enhancement)
