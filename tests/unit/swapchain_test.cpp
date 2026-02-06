#include <gtest/gtest.h>
#include "kinect_xr/runtime.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

using namespace kinect_xr;

class SwapchainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create instance
        XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strncpy(instanceInfo.applicationInfo.applicationName, "Swapchain Test", XR_MAX_APPLICATION_NAME_SIZE);

        XrResult result = KinectXRRuntime::getInstance().createInstance(&instanceInfo, &instance_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Get system
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        result = KinectXRRuntime::getInstance().getSystem(instance_, &systemInfo, &systemId_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Create session (need Metal binding - use nullptr for unit tests)
        XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
        metalBinding.commandQueue = reinterpret_cast<void*>(0x1);  // Fake pointer for unit test

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
    }

    XrInstance instance_{XR_NULL_HANDLE};
    XrSystemId systemId_{XR_NULL_SYSTEM_ID};
    XrSession session_{XR_NULL_HANDLE};
};

// M1 Tests: Handle Validation

TEST_F(SwapchainTest, InvalidSwapchainHandle) {
    XrSwapchain fakeHandle = reinterpret_cast<XrSwapchain>(0x99999);
    EXPECT_FALSE(KinectXRRuntime::getInstance().isValidSwapchain(fakeHandle));
}

TEST_F(SwapchainTest, NullSwapchainHandle) {
    EXPECT_FALSE(KinectXRRuntime::getInstance().isValidSwapchain(XR_NULL_HANDLE));
}

TEST_F(SwapchainTest, GetSwapchainDataInvalidHandle) {
    XrSwapchain fakeHandle = reinterpret_cast<XrSwapchain>(0x99999);
    SwapchainData* data = KinectXRRuntime::getInstance().getSwapchainData(fakeHandle);
    EXPECT_EQ(data, nullptr);
}

// M2 Tests: Format Enumeration

TEST_F(SwapchainTest, EnumerateFormats_CountQuery) {
    uint32_t formatCount = 0;
    XrResult result = KinectXRRuntime::getInstance().enumerateSwapchainFormats(session_, 0, &formatCount, nullptr);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(formatCount, 1u);  // Only BGRA8Unorm
}

TEST_F(SwapchainTest, EnumerateFormats_GetFormats) {
    uint32_t formatCount = 0;
    XrResult result = KinectXRRuntime::getInstance().enumerateSwapchainFormats(session_, 0, &formatCount, nullptr);
    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_EQ(formatCount, 1u);

    int64_t format = 0;
    result = KinectXRRuntime::getInstance().enumerateSwapchainFormats(session_, 1, &formatCount, &format);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(formatCount, 1u);
    EXPECT_EQ(format, 80);  // MTLPixelFormatBGRA8Unorm
}

TEST_F(SwapchainTest, EnumerateFormats_InvalidSession) {
    XrSession fakeSession = reinterpret_cast<XrSession>(0x99999);
    uint32_t formatCount = 0;
    XrResult result = KinectXRRuntime::getInstance().enumerateSwapchainFormats(fakeSession, 0, &formatCount, nullptr);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(SwapchainTest, EnumerateFormats_NullOutput) {
    XrResult result = KinectXRRuntime::getInstance().enumerateSwapchainFormats(session_, 0, nullptr, nullptr);
    EXPECT_EQ(result, XR_ERROR_VALIDATION_FAILURE);
}

TEST_F(SwapchainTest, EnumerateFormats_NullFormatsWithCapacity) {
    uint32_t formatCount = 0;
    // Providing capacity > 0 but formats=nullptr should fail
    XrResult result = KinectXRRuntime::getInstance().enumerateSwapchainFormats(session_, 1, &formatCount, nullptr);
    EXPECT_EQ(result, XR_ERROR_VALIDATION_FAILURE);
}

// M3 Tests: Swapchain Lifecycle (mock, no real Metal)

TEST_F(SwapchainTest, CreateSwapchain_Success) {
    XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo.format = 80;  // BGRA8Unorm
    createInfo.width = 640;
    createInfo.height = 480;
    createInfo.sampleCount = 1;
    createInfo.arraySize = 1;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo, &swapchain);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_NE(swapchain, XR_NULL_HANDLE);

    // Verify swapchain is valid
    EXPECT_TRUE(KinectXRRuntime::getInstance().isValidSwapchain(swapchain));

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, CreateSwapchain_InvalidFormat) {
    XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo.format = 999;  // Invalid format
    createInfo.width = 640;
    createInfo.height = 480;
    createInfo.sampleCount = 1;
    createInfo.arraySize = 1;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo, &swapchain);
    EXPECT_EQ(result, XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED);
}

TEST_F(SwapchainTest, CreateSwapchain_OversizedDimensions) {
    XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo.format = 80;
    createInfo.width = 4096;  // Larger than Kinect's 640
    createInfo.height = 4096;
    createInfo.sampleCount = 1;
    createInfo.arraySize = 1;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo, &swapchain);
    EXPECT_EQ(result, XR_ERROR_SIZE_INSUFFICIENT);
}

TEST_F(SwapchainTest, CreateSwapchain_InvalidSampleCount) {
    XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo.format = 80;
    createInfo.width = 640;
    createInfo.height = 480;
    createInfo.sampleCount = 4;  // MSAA not supported
    createInfo.arraySize = 1;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createSwapchain(session_, &createInfo, &swapchain);
    EXPECT_EQ(result, XR_ERROR_FEATURE_UNSUPPORTED);
}

TEST_F(SwapchainTest, CreateSwapchain_InvalidSession) {
    XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    createInfo.format = 80;
    createInfo.width = 640;
    createInfo.height = 480;
    createInfo.sampleCount = 1;
    createInfo.arraySize = 1;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    XrSession fakeSession = reinterpret_cast<XrSession>(0x99999);
    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createSwapchain(fakeSession, &createInfo, &swapchain);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(SwapchainTest, DestroySwapchain_Success) {
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

    // Destroy it
    result = KinectXRRuntime::getInstance().destroySwapchain(swapchain);
    EXPECT_EQ(result, XR_SUCCESS);

    // Should no longer be valid
    EXPECT_FALSE(KinectXRRuntime::getInstance().isValidSwapchain(swapchain));
}

TEST_F(SwapchainTest, DestroySwapchain_InvalidHandle) {
    XrSwapchain fakeHandle = reinterpret_cast<XrSwapchain>(0x99999);
    XrResult result = KinectXRRuntime::getInstance().destroySwapchain(fakeHandle);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(SwapchainTest, EnumerateSwapchainImages_CountQuery) {
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

    // Query image count
    uint32_t imageCount = 0;
    result = KinectXRRuntime::getInstance().enumerateSwapchainImages(swapchain, 0, &imageCount, nullptr);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(imageCount, 3u);  // Triple buffering

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, EnumerateSwapchainImages_GetImages) {
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

    // Get images
    XrSwapchainImageMetalKHR images[3] = {};
    for (auto& img : images) {
        img.type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
    }
    uint32_t imageCount = 3;
    result = KinectXRRuntime::getInstance().enumerateSwapchainImages(
        swapchain, 3, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(images));
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(imageCount, 3u);

    // For M3, images should be nullptr (no real Metal yet)
    for (const auto& img : images) {
        EXPECT_EQ(img.texture, nullptr);
    }

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, EnumerateSwapchainImages_InvalidHandle) {
    XrSwapchain fakeHandle = reinterpret_cast<XrSwapchain>(0x99999);
    uint32_t imageCount = 0;
    XrResult result = KinectXRRuntime::getInstance().enumerateSwapchainImages(fakeHandle, 0, &imageCount, nullptr);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

// M5 Tests: Image Acquire/Wait/Release

TEST_F(SwapchainTest, AcquireSwapchainImage_Success) {
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

    // Acquire first image
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t index = 999;
    result = KinectXRRuntime::getInstance().acquireSwapchainImage(swapchain, &acquireInfo, &index);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(index, 0u);  // Should get image 0 first

    // Release image
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    result = KinectXRRuntime::getInstance().releaseSwapchainImage(swapchain, &releaseInfo);
    EXPECT_EQ(result, XR_SUCCESS);

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, AcquireSwapchainImage_Cycling) {
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

    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};

    // Acquire and release 5 times, verifying cycling: 0→1→2→0→1
    uint32_t expectedIndices[] = {0, 1, 2, 0, 1};
    for (uint32_t expected : expectedIndices) {
        uint32_t index = 999;
        result = KinectXRRuntime::getInstance().acquireSwapchainImage(swapchain, &acquireInfo, &index);
        ASSERT_EQ(result, XR_SUCCESS);
        EXPECT_EQ(index, expected) << "Expected image " << expected;

        result = KinectXRRuntime::getInstance().releaseSwapchainImage(swapchain, &releaseInfo);
        ASSERT_EQ(result, XR_SUCCESS);
    }

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, AcquireSwapchainImage_DoubleAcquire) {
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

    // Acquire first image
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t index = 999;
    result = KinectXRRuntime::getInstance().acquireSwapchainImage(swapchain, &acquireInfo, &index);
    ASSERT_EQ(result, XR_SUCCESS);

    // Try to acquire again without releasing - should fail
    result = KinectXRRuntime::getInstance().acquireSwapchainImage(swapchain, &acquireInfo, &index);
    EXPECT_EQ(result, XR_ERROR_CALL_ORDER_INVALID);

    // Release and cleanup
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    KinectXRRuntime::getInstance().releaseSwapchainImage(swapchain, &releaseInfo);
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, WaitSwapchainImage_Success) {
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

    // Acquire image
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t index = 0;
    result = KinectXRRuntime::getInstance().acquireSwapchainImage(swapchain, &acquireInfo, &index);
    ASSERT_EQ(result, XR_SUCCESS);

    // Wait for image (should succeed immediately)
    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    result = KinectXRRuntime::getInstance().waitSwapchainImage(swapchain, &waitInfo);
    EXPECT_EQ(result, XR_SUCCESS);

    // Release and cleanup
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    KinectXRRuntime::getInstance().releaseSwapchainImage(swapchain, &releaseInfo);
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, WaitSwapchainImage_WithoutAcquire) {
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

    // Try to wait without acquiring - should fail
    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = 0;
    result = KinectXRRuntime::getInstance().waitSwapchainImage(swapchain, &waitInfo);
    EXPECT_EQ(result, XR_ERROR_CALL_ORDER_INVALID);

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, ReleaseSwapchainImage_WithoutAcquire) {
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

    // Try to release without acquiring - should fail
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    result = KinectXRRuntime::getInstance().releaseSwapchainImage(swapchain, &releaseInfo);
    EXPECT_EQ(result, XR_ERROR_CALL_ORDER_INVALID);

    // Cleanup
    KinectXRRuntime::getInstance().destroySwapchain(swapchain);
}

TEST_F(SwapchainTest, AcquireSwapchainImage_InvalidHandle) {
    XrSwapchain fakeHandle = reinterpret_cast<XrSwapchain>(0x99999);
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t index = 0;
    XrResult result = KinectXRRuntime::getInstance().acquireSwapchainImage(fakeHandle, &acquireInfo, &index);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}
