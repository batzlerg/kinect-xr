#include <gtest/gtest.h>
#include "kinect_xr/runtime.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

using namespace kinect_xr;

class DepthLayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create instance
        XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strncpy(instanceInfo.applicationInfo.applicationName, "Depth Layer Test", XR_MAX_APPLICATION_NAME_SIZE);

        XrResult result = KinectXRRuntime::getInstance().createInstance(&instanceInfo, &instance_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Get system
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        result = KinectXRRuntime::getInstance().getSystem(instance_, &systemInfo, &systemId_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Create session (with fake Metal binding)
        XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
        metalBinding.commandQueue = reinterpret_cast<void*>(0x1);

        XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionInfo.next = &metalBinding;
        sessionInfo.systemId = systemId_;

        result = KinectXRRuntime::getInstance().createSession(instance_, &sessionInfo, &session_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Create color swapchain
        XrSwapchainCreateInfo colorSwapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        colorSwapchainInfo.format = 80;  // BGRA8Unorm
        colorSwapchainInfo.width = 640;
        colorSwapchainInfo.height = 480;
        colorSwapchainInfo.sampleCount = 1;
        colorSwapchainInfo.arraySize = 1;
        colorSwapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        result = KinectXRRuntime::getInstance().createSwapchain(session_, &colorSwapchainInfo, &colorSwapchain_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Create depth swapchain
        XrSwapchainCreateInfo depthSwapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        depthSwapchainInfo.format = 13;  // R16Uint
        depthSwapchainInfo.width = 640;
        depthSwapchainInfo.height = 480;
        depthSwapchainInfo.sampleCount = 1;
        depthSwapchainInfo.arraySize = 1;
        depthSwapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        result = KinectXRRuntime::getInstance().createSwapchain(session_, &depthSwapchainInfo, &depthSwapchain_);
        ASSERT_EQ(result, XR_SUCCESS);
    }

    void TearDown() override {
        if (colorSwapchain_) {
            KinectXRRuntime::getInstance().destroySwapchain(colorSwapchain_);
        }
        if (depthSwapchain_) {
            KinectXRRuntime::getInstance().destroySwapchain(depthSwapchain_);
        }
        if (session_) {
            KinectXRRuntime::getInstance().destroySession(session_);
        }
        if (instance_) {
            KinectXRRuntime::getInstance().destroyInstance(instance_);
        }
    }

    // Helper to begin a frame
    void beginFrame() {
        SessionData* sessionData = KinectXRRuntime::getInstance().getSessionData(session_);
        ASSERT_NE(sessionData, nullptr);
        sessionData->state = SessionState::FOCUSED;

        XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        XrResult result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
        ASSERT_EQ(result, XR_SUCCESS);
    }

    XrInstance instance_{XR_NULL_HANDLE};
    XrSystemId systemId_{XR_NULL_SYSTEM_ID};
    XrSession session_{XR_NULL_HANDLE};
    XrSwapchain colorSwapchain_{XR_NULL_HANDLE};
    XrSwapchain depthSwapchain_{XR_NULL_HANDLE};
};

// M6 Tests: Depth Layer Parsing

TEST_F(DepthLayerTest, EndFrame_WithoutDepthLayer) {
    beginFrame();

    // Create projection layer without depth
    XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer.next = nullptr;
    projectionLayer.space = XR_NULL_HANDLE;  // Simplified for test
    projectionLayer.viewCount = 0;
    projectionLayer.views = nullptr;

    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 0;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    XrResult result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(DepthLayerTest, EndFrame_WithDepthLayer) {
    beginFrame();

    // Create depth info
    XrCompositionLayerDepthInfoKHR depthInfo{XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
    depthInfo.next = nullptr;
    depthInfo.subImage.swapchain = depthSwapchain_;
    depthInfo.subImage.imageRect.offset = {0, 0};
    depthInfo.subImage.imageRect.extent = {640, 480};
    depthInfo.subImage.imageArrayIndex = 0;
    depthInfo.minDepth = 0.0f;
    depthInfo.maxDepth = 1.0f;
    depthInfo.nearZ = 0.1f;
    depthInfo.farZ = 100.0f;

    // Create projection layer with depth in next chain
    XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer.next = &depthInfo;
    projectionLayer.space = XR_NULL_HANDLE;
    projectionLayer.viewCount = 0;
    projectionLayer.views = nullptr;

    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 0;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    XrResult result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(DepthLayerTest, EndFrame_InvalidDepthSwapchain) {
    beginFrame();

    // Create depth info with invalid swapchain handle
    XrCompositionLayerDepthInfoKHR depthInfo{XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
    depthInfo.next = nullptr;
    depthInfo.subImage.swapchain = reinterpret_cast<XrSwapchain>(0x99999);  // Invalid
    depthInfo.subImage.imageRect.offset = {0, 0};
    depthInfo.subImage.imageRect.extent = {640, 480};
    depthInfo.subImage.imageArrayIndex = 0;
    depthInfo.minDepth = 0.0f;
    depthInfo.maxDepth = 1.0f;
    depthInfo.nearZ = 0.1f;
    depthInfo.farZ = 100.0f;

    XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer.next = &depthInfo;
    projectionLayer.space = XR_NULL_HANDLE;
    projectionLayer.viewCount = 0;
    projectionLayer.views = nullptr;

    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 0;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    XrResult result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(DepthLayerTest, EndFrame_WrongDepthSwapchainFormat) {
    beginFrame();

    // Try to use color swapchain as depth
    XrCompositionLayerDepthInfoKHR depthInfo{XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
    depthInfo.next = nullptr;
    depthInfo.subImage.swapchain = colorSwapchain_;  // Wrong format (BGRA8Unorm)
    depthInfo.subImage.imageRect.offset = {0, 0};
    depthInfo.subImage.imageRect.extent = {640, 480};
    depthInfo.subImage.imageArrayIndex = 0;
    depthInfo.minDepth = 0.0f;
    depthInfo.maxDepth = 1.0f;
    depthInfo.nearZ = 0.1f;
    depthInfo.farZ = 100.0f;

    XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer.next = &depthInfo;
    projectionLayer.space = XR_NULL_HANDLE;
    projectionLayer.viewCount = 0;
    projectionLayer.views = nullptr;

    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 0;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    XrResult result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED);
}

TEST_F(DepthLayerTest, EndFrame_MultipleLayers) {
    beginFrame();

    // Create two projection layers (though Kinect only uses one)
    XrCompositionLayerProjection projectionLayer1{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer1.next = nullptr;
    projectionLayer1.space = XR_NULL_HANDLE;
    projectionLayer1.viewCount = 0;
    projectionLayer1.views = nullptr;

    XrCompositionLayerProjection projectionLayer2{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projectionLayer2.next = nullptr;
    projectionLayer2.space = XR_NULL_HANDLE;
    projectionLayer2.viewCount = 0;
    projectionLayer2.views = nullptr;

    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer1),
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer2)
    };

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 0;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 2;
    endInfo.layers = layers;

    XrResult result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_SUCCESS);
}
