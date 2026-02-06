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
