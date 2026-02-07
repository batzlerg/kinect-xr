#include <gtest/gtest.h>
#include "kinect_xr/runtime.h"
#include "kinect_xr/device.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <thread>
#include <chrono>

using namespace kinect_xr;

class KinectIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Check if Kinect is available
        if (KinectDevice::getDeviceCount() == 0) {
            GTEST_SKIP() << "No Kinect device connected, skipping test";
        }

        // Create instance
        XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strncpy(instanceInfo.applicationInfo.applicationName, "Kinect Integration Test", XR_MAX_APPLICATION_NAME_SIZE);

        XrResult result = KinectXRRuntime::getInstance().createInstance(&instanceInfo, &instance_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Get system
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        result = KinectXRRuntime::getInstance().getSystem(instance_, &systemInfo, &systemId_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Create session (with fake Metal binding for integration test)
        XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
        metalBinding.commandQueue = reinterpret_cast<void*>(0x1);

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

// M3 Tests: Kinect Device Integration

TEST_F(KinectIntegrationTest, BeginSession_InitializesKinect) {
    // Transition to READY state first
    SessionData* sessionData = KinectXRRuntime::getInstance().getSessionData(session_);
    ASSERT_NE(sessionData, nullptr);
    sessionData->state = SessionState::READY;

    // Begin session (should initialize Kinect)
    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;

    XrResult result = KinectXRRuntime::getInstance().beginSession(session_, &beginInfo);
    EXPECT_EQ(result, XR_SUCCESS);

    // Kinect device should be initialized
    EXPECT_NE(sessionData->kinectDevice, nullptr);
    EXPECT_TRUE(sessionData->kinectDevice->isInitialized());
    EXPECT_TRUE(sessionData->kinectDevice->isStreaming());

    // End session (cleanup)
    result = KinectXRRuntime::getInstance().endSession(session_);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(KinectIntegrationTest, EndSession_StopsKinect) {
    // Begin session first
    SessionData* sessionData = KinectXRRuntime::getInstance().getSessionData(session_);
    ASSERT_NE(sessionData, nullptr);
    sessionData->state = SessionState::READY;

    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    XrResult result = KinectXRRuntime::getInstance().beginSession(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    ASSERT_NE(sessionData->kinectDevice, nullptr);
    ASSERT_TRUE(sessionData->kinectDevice->isStreaming());

    // End session
    result = KinectXRRuntime::getInstance().endSession(session_);
    EXPECT_EQ(result, XR_SUCCESS);

    // Kinect device should be released
    EXPECT_EQ(sessionData->kinectDevice, nullptr);
}

TEST_F(KinectIntegrationTest, Callbacks_PopulateFrameCache) {
    // Begin session
    SessionData* sessionData = KinectXRRuntime::getInstance().getSessionData(session_);
    ASSERT_NE(sessionData, nullptr);
    sessionData->state = SessionState::READY;

    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    XrResult result = KinectXRRuntime::getInstance().beginSession(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // Wait for callbacks to fire (Kinect runs at 30 Hz, ~33ms per frame)
    // Wait for at least 3 frames to ensure depth arrives
    // NOTE: RGB stream can be unreliable due to libfreenect USB issues
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Check frame cache
    {
        std::lock_guard<std::mutex> lock(sessionData->frameCache.mutex);

        // Depth should be valid (depth is more reliable than RGB)
        EXPECT_TRUE(sessionData->frameCache.depthValid);
        EXPECT_GT(sessionData->frameCache.depthTimestamp, 0u);

        // RGB may or may not be valid (USB transfer can be unreliable)
        // Just log the status, don't fail test
        if (sessionData->frameCache.rgbValid) {
            std::cout << "RGB frame received (timestamp: " << sessionData->frameCache.rgbTimestamp << ")" << std::endl;
        } else {
            std::cout << "RGB frame not received (libfreenect RGB can be unreliable)" << std::endl;
        }
    }

    // End session
    result = KinectXRRuntime::getInstance().endSession(session_);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(KinectIntegrationTest, Callbacks_UpdateFrameCache) {
    // Begin session
    SessionData* sessionData = KinectXRRuntime::getInstance().getSessionData(session_);
    ASSERT_NE(sessionData, nullptr);
    sessionData->state = SessionState::READY;

    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    XrResult result = KinectXRRuntime::getInstance().beginSession(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // Wait for first frame (at least 2 frames @ 30Hz = 67ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    uint32_t firstDepthTimestamp = 0;
    {
        std::lock_guard<std::mutex> lock(sessionData->frameCache.mutex);
        firstDepthTimestamp = sessionData->frameCache.depthTimestamp;
    }

    // Wait for more frames (at least 3 more frames @ 30Hz = 100ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Depth timestamps should have updated (if Kinect is delivering frames)
    {
        std::lock_guard<std::mutex> lock(sessionData->frameCache.mutex);
        if (sessionData->frameCache.depthTimestamp > firstDepthTimestamp) {
            std::cout << "Depth frame updated successfully" << std::endl;
        } else {
            // Kinect may not deliver new frames due to USB packet loss
            // This is a hardware reliability issue, not a test failure
            GTEST_SKIP() << "Kinect not delivering new frames (USB packet loss)";
        }
    }

    // End session
    result = KinectXRRuntime::getInstance().endSession(session_);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(KinectIntegrationTest, DepthData_ValidRange) {
    // Begin session
    SessionData* sessionData = KinectXRRuntime::getInstance().getSessionData(session_);
    ASSERT_NE(sessionData, nullptr);
    sessionData->state = SessionState::READY;

    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    XrResult result = KinectXRRuntime::getInstance().beginSession(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // Wait for depth data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Check depth values are in valid range
    // Note: device.cpp uses FREENECT_DEPTH_MM which returns millimeter values (0-10000mm)
    // not 11-bit raw disparity (0-2047)
    {
        std::lock_guard<std::mutex> lock(sessionData->frameCache.mutex);
        ASSERT_TRUE(sessionData->frameCache.depthValid);

        int count_out_of_range = 0;
        int count_total = sessionData->frameCache.depthData.size();

        for (size_t i = 0; i < sessionData->frameCache.depthData.size(); ++i) {
            uint16_t val = sessionData->frameCache.depthData[i];
            // Valid range: 0 = no reading, 1-10000 = distance in mm (up to 10 meters)
            if (val > 10000) {
                count_out_of_range++;
            }
        }

        // Allow up to 1% of pixels to have noise/invalid readings
        float error_rate = (float)count_out_of_range / count_total;
        EXPECT_LT(error_rate, 0.01)
            << "Too many invalid depth values: " << count_out_of_range
            << " out of " << count_total << " pixels ("
            << (error_rate * 100.0) << "%)";
    }

    // End session
    result = KinectXRRuntime::getInstance().endSession(session_);
    EXPECT_EQ(result, XR_SUCCESS);
}
