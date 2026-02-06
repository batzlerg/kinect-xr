#include "kinect_xr/runtime.h"
#include "kinect_xr/metal_helper.h"
#include <cstring>

namespace kinect_xr {

// Convert RGB888 to BGRA8888 (Metal's native format on macOS)
// Input: RGB888 (640x480x3 bytes)
// Output: BGRA8888 (640x480x4 bytes)
void convertRGB888toBGRA8888(const uint8_t* rgb, uint8_t* bgra, uint32_t width, uint32_t height) {
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t rgbIdx = (y * width + x) * 3;
            uint32_t bgraIdx = (y * width + x) * 4;

            bgra[bgraIdx + 0] = rgb[rgbIdx + 2];  // B
            bgra[bgraIdx + 1] = rgb[rgbIdx + 1];  // G
            bgra[bgraIdx + 2] = rgb[rgbIdx + 0];  // R
            bgra[bgraIdx + 3] = 255;              // A (opaque)
        }
    }
}

// Upload RGB frame from cache to swapchain texture
// Returns true if upload succeeded, false if no frame available or upload failed
bool uploadRGBTexture(SessionData* sessionData, SwapchainData* swapchainData) {
    // Validate inputs
    if (!sessionData || !swapchainData) {
        return false;
    }

    // Only upload to color swapchains (BGRA8Unorm = 80)
    if (swapchainData->format != 80) {
        return false;
    }

    // No image acquired yet
    if (!swapchainData->imageAcquired) {
        return false;
    }

    // Get current texture
    void* texture = swapchainData->metalTextures[swapchainData->currentImageIndex];
    if (!texture) {
        return false;
    }

    // Check if RGB frame is available
    bool hasRGB = false;
    std::vector<uint8_t> rgbCopy;
    {
        std::lock_guard<std::mutex> lock(sessionData->frameCache.mutex);
        if (sessionData->frameCache.rgbValid) {
            hasRGB = true;
            rgbCopy = sessionData->frameCache.rgbData;  // Copy to avoid holding lock during upload
        }
    }

    if (!hasRGB) {
        return false;  // No RGB frame available
    }

    // Convert RGB888 to BGRA8888
    std::vector<uint8_t> bgra(640 * 480 * 4);
    convertRGB888toBGRA8888(rgbCopy.data(), bgra.data(), 640, 480);

    // Upload to Metal texture
    bool uploadSuccess = metal::uploadTextureData(
        texture,
        bgra.data(),
        640 * 4,  // bytes per row
        640,
        480
    );

    return uploadSuccess;
}

// Upload depth frame from cache to swapchain texture
// Returns true if upload succeeded, false if no frame available or upload failed
bool uploadDepthTexture(SessionData* sessionData, SwapchainData* swapchainData) {
    // Validate inputs
    if (!sessionData || !swapchainData) {
        return false;
    }

    // Only upload to depth swapchains (R16Uint = 13)
    if (swapchainData->format != 13) {
        return false;
    }

    // No image acquired yet
    if (!swapchainData->imageAcquired) {
        return false;
    }

    // Get current texture
    void* texture = swapchainData->metalTextures[swapchainData->currentImageIndex];
    if (!texture) {
        return false;
    }

    // Check if depth frame is available
    bool hasDepth = false;
    std::vector<uint16_t> depthCopy;
    {
        std::lock_guard<std::mutex> lock(sessionData->frameCache.mutex);
        if (sessionData->frameCache.depthValid) {
            hasDepth = true;
            depthCopy = sessionData->frameCache.depthData;  // Copy to avoid holding lock during upload
        }
    }

    if (!hasDepth) {
        return false;  // No depth frame available
    }

    // Upload to Metal texture (R16Uint, 11-bit Kinect depth passthrough)
    bool uploadSuccess = metal::uploadTextureData(
        texture,
        depthCopy.data(),
        640 * 2,  // bytes per row (uint16_t = 2 bytes)
        640,
        480
    );

    return uploadSuccess;
}

// Upload textures for all swapchains belonging to a session
// Call this after xrAcquireSwapchainImage, before rendering
void uploadSessionTextures(SessionData* sessionData,
                           const std::unordered_map<XrSwapchain, std::unique_ptr<SwapchainData>>& swapchains) {
    if (!sessionData) {
        return;
    }

    // Upload to all swapchains that belong to this session
    for (const auto& [handle, swapchainData] : swapchains) {
        if (swapchainData->session != sessionData->handle) {
            continue;  // Skip swapchains from other sessions
        }

        // Upload based on format
        if (swapchainData->format == 80) {
            // Color swapchain - upload RGB
            uploadRGBTexture(sessionData, swapchainData.get());
        } else if (swapchainData->format == 13) {
            // Depth swapchain - upload depth
            uploadDepthTexture(sessionData, swapchainData.get());
        }
    }
}

} // namespace kinect_xr
