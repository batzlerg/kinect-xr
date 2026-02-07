# Kinect Depth Data Guide

Technical reference for understanding Kinect v1 depth data in the kinect-xr project.

## Depth Formats in libfreenect

libfreenect supports multiple depth formats with **very different value semantics**:

| Format | Values | Meaning | Use Case |
|--------|--------|---------|----------|
| `FREENECT_DEPTH_11BIT` | 0-2047 | Raw disparity (inverse depth!) | Low-level processing |
| `FREENECT_DEPTH_10BIT` | 0-1023 | Raw disparity (lower precision) | Memory constrained |
| `FREENECT_DEPTH_MM` | 0-10000 | Millimeters from sensor | **Recommended for 3D** |
| `FREENECT_DEPTH_REGISTERED` | 0-10000 | Millimeters, aligned to RGB | RGB-D fusion |

### Critical: Disparity vs Distance

**Raw disparity has an INVERSE relationship to distance:**

```
High disparity value → Object is CLOSE to sensor
Low disparity value  → Object is FAR from sensor
```

This is the opposite of millimeter values! The kinect-xr project previously used `FREENECT_DEPTH_11BIT` but treated values as millimeters, causing complete depth inversion.

**Fix:** Use `FREENECT_DEPTH_MM` which provides actual millimeter values where higher = farther.

### Invalid Value Handling

| Format | Invalid Value | Meaning |
|--------|---------------|---------|
| `FREENECT_DEPTH_11BIT` | 2047 | Too far / no return |
| `FREENECT_DEPTH_11BIT` | 0 | Too close / no return |
| `FREENECT_DEPTH_MM` | 0 | No valid depth data |

## Hardware Limitations (Kinect v1)

### Depth Sensing Range

| Limit | Distance | Notes |
|-------|----------|-------|
| **Minimum** | ~400-800mm | Hardware limit, not software configurable |
| **Optimal** | 800-2500mm | Best accuracy range |
| **Maximum** | ~4000mm | Accuracy degrades beyond this |
| **Absolute max** | ~8000mm | Theoretical limit, very noisy |

**Why ~800mm minimum?** The Kinect uses structured IR light projection. Objects too close cause:
1. IR pattern overlap/saturation
2. Stereo matching failure
3. Shadow regions behind close objects

The "shadow" effect you see (hand blocking background) is expected behavior - the IR projector can't see behind opaque objects.

### Near Mode (Kinect for Windows only)

The "Kinect for Windows" hardware revision supports a near mode with ~400mm minimum range. The original Xbox 360 Kinect does not support this mode.

## Three.js Coordinate Mapping

### Coordinate Systems

**Kinect sensor frame:**
- +X: Right (from sensor's perspective)
- +Y: Up
- +Z: Forward (away from sensor)

**Three.js default:**
- +X: Right
- +Y: Up
- +Z: Toward viewer (out of screen)
- -Z: Into scene (away from viewer)

### Projection Mapping

With camera at positive Z looking toward negative Z:

```javascript
// Convert depth mm to meters
const depthMeters = depth * 0.001;

// Pinhole camera projection
const px = ((x - CX) * depthMeters) / FOCAL_LENGTH_X;
const py = -((y - CY) * depthMeters) / FOCAL_LENGTH_Y;
const pz = -depthMeters;  // NEGATIVE - into scene

// Camera at z=2.5 sees:
// - Close objects (small depthMeters) → small |pz| → closer to camera
// - Far objects (large depthMeters) → large |pz| → farther from camera
```

### Grid Alignment

The grid should be positioned within the depth sensing range:

```javascript
// Depth range: z=-0.8m to z=-4.0m (negative = into scene)
// Grid at z=-2m is center of typical range
gridHelper.position.set(0, -0.5, -2);
controls.target.set(0, 0, -2);  // Look at depth center
```

## Frustum Culling Issue

Three.js uses bounding sphere calculations for frustum culling. With dynamic point clouds:
- Bounding sphere may not update correctly
- Entire point cloud can disappear at certain angles

**Fix:** Disable frustum culling for point clouds:

```javascript
pointCloud.frustumCulled = false;
```

## Color Mapping

HSL color wheel for depth visualization:
- **hue=0°** → Red (close objects)
- **hue=120°** → Green
- **hue=240°** → Blue (far objects)

```javascript
// Normalize to actual scene range (not theoretical max)
const normalizedDepth = (depth - minDepth) / (maxDepth - minDepth);
const hue = normalizedDepth * 240;  // 0=red, 240=blue
```

## USB Device Access Issues

libfreenect/libusb can fail to acquire the device if:
1. Previous process didn't release it cleanly
2. Another application is using the Kinect
3. USB subsystem needs reset

**Symptoms:**
```
send_cmd: Bad cmd 03 != 16
freenect_camera_init(): Failed to fetch registration pad info
```

**Workarounds:**
1. Run the start script again (usually works)
2. Unplug and replug the Kinect
3. Kill any orphaned bridge processes: `pkill -f kinect_bridge`
4. Add retry logic to startup script

## Quick Reference

| Question | Answer |
|----------|--------|
| What format to use? | `FREENECT_DEPTH_MM` |
| Min sensing distance? | ~800mm (hardware limit) |
| Why objects disappear close? | IR saturation + shadow |
| Why points disappear on rotate? | Frustum culling - disable it |
| Why grid is offset? | Grid at z=1, points at z=-2, need to align |
| Why startup fails sometimes? | USB device not released, retry |
