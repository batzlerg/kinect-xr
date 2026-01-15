# Architecture

## Overview

C++ OpenXR runtime layered on libfreenect for Kinect 1 hardware access.

## Components

### Core
- `src/` - OpenXR runtime implementation
- `include/` - Header files

### Build
- CMake build system
- libfreenect dependency

## Data Flow

```
Kinect USB → libfreenect → OpenXR Runtime → (WebSocket) → Browser
```

## Key Decisions

| Decision | Rationale | Date |
|----------|-----------|------|
| libfreenect | Stable Kinect 1 driver | 2025 |
| OpenXR | Standard XR API | 2025 |
| C++ | Performance, libfreenect compat | 2025 |

## Dependencies

- libfreenect
- OpenXR loader
- CMake

## Development

### Setup
```bash
cmake -B build -S .
cmake --build build
```

### Testing
```bash
./test.sh
```
