#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <gtest/gtest.h>
#include "kinect_xr/runtime.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

using namespace kinect_xr;

class SwapchainMetalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create real Metal device and command queue
        metalDevice_ = MTLCreateSystemDefaultDevice();
        ASSERT_NE(metalDevice_, nil) << "Metal is not available on this system";

        metalCommandQueue_ = [metalDevice_ newCommandQueue];
        ASSERT_NE(metalCommandQueue_, nil) << "Failed to create Metal command queue";

        // Create OpenXR instance
        XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strncpy(instanceInfo.applicationInfo.applicationName, "Metal Swapchain Test", XR_MAX_APPLICATION_NAME_SIZE);

        XrResult result = KinectXRRuntime::getInstance().createInstance(&instanceInfo, &instance_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Get system
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        result = KinectXRRuntime::getInstance().getSystem(instance_, &systemInfo, &systemId_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Create session with real Metal binding
        XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
        metalBinding.commandQueue = (__bridge void*)metalCommandQueue_;

        XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionInfo.next = &metalBinding;
        sessionInfo.systemId = systemId_;

        result = KinectXRRuntime::getInstance().createSession(instance_, &sessionInfo, &session_);
        ASSERT_EQ(result, XR_SUCCESS);
    }

    void TearDown() override {
        if (session_) {
            KinectXRRuntime::getInstance().destroySession(session_);
        }
        if (instance_) {
            KinectXRRuntime::getInstance().destroyInstance(instance_);
        }

        if (metalCommandQueue_) {
            metalCommandQueue_ = nil;
        }
        if (metalDevice_) {
            metalDevice_ = nil;
        }
    }

    id<MTLDevice> metalDevice_;
    id<MTLCommandQueue> metalCommandQueue_;
    XrInstance instance_{XR_NULL_HANDLE};
    XrSystemId systemId_{XR_NULL_SYSTEM_ID};
    XrSession session_{XR_NULL_HANDLE};
};

// M4 Integration Tests: Real Metal Texture Creation

TEST_F(SwapchainMetalTest, CreateSwapchain_CreatesRealMetalTextures) {
    XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo.format = 80;  // BGRA8Unorm
    createInfo.width = 640;
    createInfo.height = 480;
    createInfo.sampleCount = 1;
    createInfo.arraySize = 1;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo, &swapchain);
    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_NE(swapchain, XR_NULL_HANDLE);

    // Get swapchain images
    XrSwapchainImageMetalKHR images[3] = {};
    for (auto& img : images) {
        img.type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
    }
    uint32_t imageCount = 3;
    result = KinectXRRuntime::getInstance().enumerateSwapchainImages(
        swapchain, 3, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(images));
    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_EQ(imageCount, 3u);

    // Verify we got real Metal textures (not nullptr)
    for (uint32_t i = 0; i < 3; ++i) {
        ASSERT_NE(images[i].texture, nullptr) << "Image " << i << " texture is null";

        // Cast to MTLTexture and verify properties
        id<MTLTexture> texture = (__bridge id<MTLTexture>)images[i].texture;
        EXPECT_EQ(texture.width, 640u);
        EXPECT_EQ(texture.height, 480u);
        EXPECT_EQ(texture.pixelFormat, MTLPixelFormatBGRA8Unorm);
        EXPECT_EQ(texture.textureType, MTLTextureType2D);
    }

    // Cleanup
    result = KinectXRRuntime::getInstance().destroySwapchain(swapchain);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(SwapchainMetalTest, DestroySwapchain_ReleasesMetalTextures) {
    // Create swapchain
    XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo.format = 80;
    createInfo.width = 640;
    createInfo.height = 480;
    createInfo.sampleCount = 1;
    createInfo.arraySize = 1;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo, &swapchain);
    ASSERT_EQ(result, XR_SUCCESS);

    // Get texture pointers before destroy
    XrSwapchainImageMetalKHR images[3] = {};
    for (auto& img : images) {
        img.type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
    }
    uint32_t imageCount = 3;
    result = KinectXRRuntime::getInstance().enumerateSwapchainImages(
        swapchain, 3, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(images));
    ASSERT_EQ(result, XR_SUCCESS);

    // Verify textures exist
    for (const auto& img : images) {
        ASSERT_NE(img.texture, nullptr);
    }

    // Destroy swapchain (should release Metal textures)
    result = KinectXRRuntime::getInstance().destroySwapchain(swapchain);
    EXPECT_EQ(result, XR_SUCCESS);

    // Note: We can't easily verify textures are released (Metal manages memory)
    // This test mainly ensures no crashes during cleanup
}

TEST_F(SwapchainMetalTest, MultipleSwapchains_IndependentTextures) {
    // Create two swapchains with different sizes
    XrSwapchainCreateInfo createInfo1{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo1.format = 80;
    createInfo1.width = 640;
    createInfo1.height = 480;
    createInfo1.sampleCount = 1;
    createInfo1.arraySize = 1;
    createInfo1.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchainCreateInfo createInfo2{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo2.format = 80;
    createInfo2.width = 320;
    createInfo2.height = 240;
    createInfo2.sampleCount = 1;
    createInfo2.arraySize = 1;
    createInfo2.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchain swapchain1 = XR_NULL_HANDLE;
    XrSwapchain swapchain2 = XR_NULL_HANDLE;

    XrResult result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo1, &swapchain1);
    ASSERT_EQ(result, XR_SUCCESS);

    result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo2, &swapchain2);
    ASSERT_EQ(result, XR_SUCCESS);

    // Get images from both swapchains
    XrSwapchainImageMetalKHR images1[3] = {};
    XrSwapchainImageMetalKHR images2[3] = {};
    for (int i = 0; i < 3; ++i) {
        images1[i].type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
        images2[i].type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
    }

    uint32_t count1 = 3, count2 = 3;
    result = KinectXRRuntime::getInstance().enumerateSwapchainImages(
        swapchain1, 3, &count1, reinterpret_cast<XrSwapchainImageBaseHeader*>(images1));
    ASSERT_EQ(result, XR_SUCCESS);

    result = KinectXRRuntime::getInstance().enumerateSwapchainImages(
        swapchain2, 3, &count2, reinterpret_cast<XrSwapchainImageBaseHeader*>(images2));
    ASSERT_EQ(result, XR_SUCCESS);

    // Verify different sizes
    id<MTLTexture> tex1 = (__bridge id<MTLTexture>)images1[0].texture;
    id<MTLTexture> tex2 = (__bridge id<MTLTexture>)images2[0].texture;

    EXPECT_EQ(tex1.width, 640u);
    EXPECT_EQ(tex1.height, 480u);
    EXPECT_EQ(tex2.width, 320u);
    EXPECT_EQ(tex2.height, 240u);

    // Verify textures are different objects
    EXPECT_NE(images1[0].texture, images2[0].texture);

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain1);
    KinectXRRuntime::getInstance().destroySwapchain(swapchain2);
}
