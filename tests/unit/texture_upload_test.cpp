#include <gtest/gtest.h>
#include "kinect_xr/runtime.h"

using namespace kinect_xr;

class TextureUploadTest : public ::testing::Test {
protected:
};

// M4 Tests: RGB888 to BGRA8888 Conversion

TEST_F(TextureUploadTest, ConvertRGB888toBGRA8888_BasicConversion) {
    // Input: 2x2 RGB image
    uint8_t rgb[12] = {
        255, 0, 0,     // Red pixel
        0, 255, 0,     // Green pixel
        0, 0, 255,     // Blue pixel
        128, 128, 128  // Gray pixel
    };

    uint8_t bgra[16] = {0};

    convertRGB888toBGRA8888(rgb, bgra, 2, 2);

    // Check red pixel (R=255, G=0, B=0 → BGRA=0,0,255,255)
    EXPECT_EQ(bgra[0], 0);    // B
    EXPECT_EQ(bgra[1], 0);    // G
    EXPECT_EQ(bgra[2], 255);  // R
    EXPECT_EQ(bgra[3], 255);  // A

    // Check green pixel (R=0, G=255, B=0 → BGRA=0,255,0,255)
    EXPECT_EQ(bgra[4], 0);    // B
    EXPECT_EQ(bgra[5], 255);  // G
    EXPECT_EQ(bgra[6], 0);    // R
    EXPECT_EQ(bgra[7], 255);  // A

    // Check blue pixel (R=0, G=0, B=255 → BGRA=255,0,0,255)
    EXPECT_EQ(bgra[8], 255);  // B
    EXPECT_EQ(bgra[9], 0);    // G
    EXPECT_EQ(bgra[10], 0);   // R
    EXPECT_EQ(bgra[11], 255); // A

    // Check gray pixel (R=128, G=128, B=128 → BGRA=128,128,128,255)
    EXPECT_EQ(bgra[12], 128); // B
    EXPECT_EQ(bgra[13], 128); // G
    EXPECT_EQ(bgra[14], 128); // R
    EXPECT_EQ(bgra[15], 255); // A
}

TEST_F(TextureUploadTest, ConvertRGB888toBGRA8888_AlphaIsOpaque) {
    uint8_t rgb[3] = {100, 150, 200};
    uint8_t bgra[4] = {0};

    convertRGB888toBGRA8888(rgb, bgra, 1, 1);

    // Alpha should always be 255 (opaque)
    EXPECT_EQ(bgra[3], 255);
}

TEST_F(TextureUploadTest, ConvertRGB888toBGRA8888_KinectDimensions) {
    // Test with Kinect's actual dimensions (640x480)
    const uint32_t width = 640;
    const uint32_t height = 480;
    std::vector<uint8_t> rgb(width * height * 3);
    std::vector<uint8_t> bgra(width * height * 4);

    // Fill with test pattern
    for (size_t i = 0; i < rgb.size(); i += 3) {
        rgb[i] = 255;     // R
        rgb[i+1] = 128;   // G
        rgb[i+2] = 64;    // B
    }

    convertRGB888toBGRA8888(rgb.data(), bgra.data(), width, height);

    // Check a few pixels
    EXPECT_EQ(bgra[0], 64);   // B
    EXPECT_EQ(bgra[1], 128);  // G
    EXPECT_EQ(bgra[2], 255);  // R
    EXPECT_EQ(bgra[3], 255);  // A

    // Check last pixel
    size_t lastPixel = (width * height - 1) * 4;
    EXPECT_EQ(bgra[lastPixel], 64);   // B
    EXPECT_EQ(bgra[lastPixel+1], 128); // G
    EXPECT_EQ(bgra[lastPixel+2], 255); // R
    EXPECT_EQ(bgra[lastPixel+3], 255); // A
}

TEST_F(TextureUploadTest, ConvertRGB888toBGRA8888_ByteOrder) {
    // Verify byte order is correct for Metal BGRA8Unorm
    uint8_t rgb[3] = {0x12, 0x34, 0x56};  // R=0x12, G=0x34, B=0x56
    uint8_t bgra[4] = {0};

    convertRGB888toBGRA8888(rgb, bgra, 1, 1);

    EXPECT_EQ(bgra[0], 0x56);  // B
    EXPECT_EQ(bgra[1], 0x34);  // G
    EXPECT_EQ(bgra[2], 0x12);  // R
    EXPECT_EQ(bgra[3], 0xFF);  // A
}

// M4 Tests: Upload Logic (without real Metal)

TEST_F(TextureUploadTest, UploadRGBTexture_RejectsNullInputs) {
    bool result = uploadRGBTexture(nullptr, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadDepthTexture_RejectsNullInputs) {
    bool result = uploadDepthTexture(nullptr, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadRGBTexture_RejectsDepthSwapchain) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 13);  // R16Uint (depth)

    bool result = uploadRGBTexture(&sessionData, &swapchainData);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadDepthTexture_RejectsColorSwapchain) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 80);  // BGRA8Unorm (color)

    bool result = uploadDepthTexture(&sessionData, &swapchainData);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadRGBTexture_RequiresAcquiredImage) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 80);
    swapchainData.imageAcquired = false;

    bool result = uploadRGBTexture(&sessionData, &swapchainData);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadDepthTexture_RequiresAcquiredImage) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 13);
    swapchainData.imageAcquired = false;

    bool result = uploadDepthTexture(&sessionData, &swapchainData);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadRGBTexture_RequiresValidFrame) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 80);
    swapchainData.imageAcquired = true;
    swapchainData.metalTextures[0] = reinterpret_cast<void*>(0x12345678);  // Fake texture

    // Frame cache has no valid RGB
    sessionData.frameCache.rgbValid = false;

    bool result = uploadRGBTexture(&sessionData, &swapchainData);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadDepthTexture_RequiresValidFrame) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 13);
    swapchainData.imageAcquired = true;
    swapchainData.metalTextures[0] = reinterpret_cast<void*>(0x12345678);  // Fake texture

    // Frame cache has no valid depth
    sessionData.frameCache.depthValid = false;

    bool result = uploadDepthTexture(&sessionData, &swapchainData);
    EXPECT_FALSE(result);
}

TEST_F(TextureUploadTest, UploadRGBTexture_SucceedsWithValidFrame) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 80);
    swapchainData.imageAcquired = true;
    swapchainData.metalTextures[0] = reinterpret_cast<void*>(0x12345678);  // Fake texture (in fake range)

    // Populate frame cache with RGB
    sessionData.frameCache.rgbValid = true;
    for (size_t i = 0; i < sessionData.frameCache.rgbData.size(); ++i) {
        sessionData.frameCache.rgbData[i] = static_cast<uint8_t>(i % 256);
    }

    bool result = uploadRGBTexture(&sessionData, &swapchainData);
    EXPECT_TRUE(result);  // Should succeed (fake texture upload)
}

TEST_F(TextureUploadTest, UploadDepthTexture_SucceedsWithValidFrame) {
    SessionData sessionData(XR_NULL_HANDLE, XR_NULL_HANDLE, 0);
    SwapchainData swapchainData(XR_NULL_HANDLE, XR_NULL_HANDLE, 640, 480, 13);
    swapchainData.imageAcquired = true;
    swapchainData.metalTextures[0] = reinterpret_cast<void*>(0x12345678);  // Fake texture (in fake range)

    // Populate frame cache with depth
    sessionData.frameCache.depthValid = true;
    for (size_t i = 0; i < sessionData.frameCache.depthData.size(); ++i) {
        sessionData.frameCache.depthData[i] = static_cast<uint16_t>(i % 2048);
    }

    bool result = uploadDepthTexture(&sessionData, &swapchainData);
    EXPECT_TRUE(result);  // Should succeed (fake texture upload)
}
