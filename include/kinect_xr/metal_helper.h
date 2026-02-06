#pragma once

#include <cstdint>

namespace kinect_xr {
namespace metal {

/**
 * Create a Metal texture
 * @param metalDevicePtr Pointer to MTLDevice (as void*)
 * @param width Texture width
 * @param height Texture height
 * @param format Metal pixel format (int64_t, e.g., 80 for BGRA8Unorm)
 * @return Pointer to MTLTexture (as void*), or nullptr on failure
 */
void* createTexture(void* metalDevicePtr, uint32_t width, uint32_t height, int64_t format);

/**
 * Release a Metal texture
 * @param texturePtr Pointer to MTLTexture (as void*)
 */
void releaseTexture(void* texturePtr);

/**
 * Get Metal device from command queue
 * @param commandQueuePtr Pointer to MTLCommandQueue (as void*)
 * @return Pointer to MTLDevice (as void*), or nullptr if invalid
 */
void* getMetalDevice(void* commandQueuePtr);

} // namespace metal
} // namespace kinect_xr
