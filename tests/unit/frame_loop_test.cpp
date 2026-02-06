#include <gtest/gtest.h>
#include "kinect_xr/runtime.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <chrono>

using namespace kinect_xr;

class FrameLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create instance
        XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strncpy(instanceInfo.applicationInfo.applicationName, "Frame Loop Test", XR_MAX_APPLICATION_NAME_SIZE);

        XrResult result = KinectXRRuntime::getInstance().createInstance(&instanceInfo, &instance_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Get system
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        result = KinectXRRuntime::getInstance().getSystem(instance_, &systemInfo, &systemId_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Create session
        XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
        metalBinding.commandQueue = reinterpret_cast<void*>(0x1);  // Fake pointer for unit test

        XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionInfo.next = &metalBinding;
        sessionInfo.systemId = systemId_;

        result = KinectXRRuntime::getInstance().createSession(instance_, &sessionInfo, &session_);
        ASSERT_EQ(result, XR_SUCCESS);

        // Begin session to enter running state
        XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
        result = KinectXRRuntime::getInstance().beginSession(session_, &beginInfo);
        ASSERT_EQ(result, XR_SUCCESS);

        // Poll events to consume state change events
        XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
        while (KinectXRRuntime::getInstance().pollEvent(instance_, &eventData) == XR_SUCCESS) {
            // Consume all events
        }
    }

    void TearDown() override {
        if (session_) {
            // End session if it's running
            KinectXRRuntime::getInstance().endSession(session_);
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

// M6 Tests: Frame Timing (xrWaitFrame)

TEST_F(FrameLoopTest, WaitFrame_Success) {
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};

    XrResult result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, &frameState);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_GT(frameState.predictedDisplayTime, 0);
    EXPECT_EQ(frameState.predictedDisplayPeriod, 33333333);  // 33.3ms in ns
    EXPECT_EQ(frameState.shouldRender, XR_TRUE);
}

TEST_F(FrameLoopTest, WaitFrame_InvalidSession) {
    XrSession fakeSession = reinterpret_cast<XrSession>(0x99999);
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};

    XrResult result = KinectXRRuntime::getInstance().waitFrame(fakeSession, &waitInfo, &frameState);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(FrameLoopTest, WaitFrame_Pacing30Hz) {
    // Call waitFrame twice and measure time between calls
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState1{XR_TYPE_FRAME_STATE};
    XrFrameState frameState2{XR_TYPE_FRAME_STATE};

    auto start = std::chrono::steady_clock::now();
    XrResult result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, &frameState1);
    ASSERT_EQ(result, XR_SUCCESS);

    result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, &frameState2);
    ASSERT_EQ(result, XR_SUCCESS);
    auto end = std::chrono::steady_clock::now();

    // Second call should have slept to maintain 30Hz (~33ms)
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(elapsed, 30);  // At least 30ms (allow some jitter)
    EXPECT_LE(elapsed, 40);  // But not too much more than 33ms
}

TEST_F(FrameLoopTest, WaitFrame_IncrementingDisplayTime) {
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState1{XR_TYPE_FRAME_STATE};
    XrFrameState frameState2{XR_TYPE_FRAME_STATE};

    XrResult result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, &frameState1);
    ASSERT_EQ(result, XR_SUCCESS);

    result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, &frameState2);
    ASSERT_EQ(result, XR_SUCCESS);

    // Display times should be increasing
    EXPECT_GT(frameState2.predictedDisplayTime, frameState1.predictedDisplayTime);

    // Difference should be roughly 33ms (in nanoseconds)
    int64_t diff = frameState2.predictedDisplayTime - frameState1.predictedDisplayTime;
    int64_t expectedDiff = 33333333;  // 33.3ms in ns
    EXPECT_GE(diff, expectedDiff - 5000000);  // Allow 5ms jitter
    EXPECT_LE(diff, expectedDiff + 5000000);
}

TEST_F(FrameLoopTest, WaitFrame_NullParameters) {
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};

    XrResult result = KinectXRRuntime::getInstance().waitFrame(session_, nullptr, &frameState);
    EXPECT_EQ(result, XR_ERROR_VALIDATION_FAILURE);

    result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, nullptr);
    EXPECT_EQ(result, XR_ERROR_VALIDATION_FAILURE);
}
