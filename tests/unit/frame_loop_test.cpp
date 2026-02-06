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

// M7 Tests: Frame State Machine (xrBeginFrame/xrEndFrame)

TEST_F(FrameLoopTest, BeginFrame_Success) {
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
    EXPECT_EQ(result, XR_SUCCESS);

    // End frame to clean up
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 1000;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
}

TEST_F(FrameLoopTest, BeginFrame_DoubleBegin) {
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // Try to begin again without ending - should fail
    result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
    EXPECT_EQ(result, XR_ERROR_CALL_ORDER_INVALID);

    // Clean up
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 1000;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
}

TEST_F(FrameLoopTest, EndFrame_Success) {
    // Begin frame first
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // End frame
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 1000;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(FrameLoopTest, EndFrame_WithoutBegin) {
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 1000;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;

    XrResult result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_ERROR_CALL_ORDER_INVALID);
}

TEST_F(FrameLoopTest, EndFrame_InvalidBlendMode) {
    // Begin frame first
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // End frame with unsupported blend mode
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = 1000;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;

    result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED);

    // Clean up with correct blend mode
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
}

TEST_F(FrameLoopTest, FullFrameLoop) {
    // Simulate typical frame loop: wait → begin → end
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    XrResult result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, &frameState);
    ASSERT_EQ(result, XR_SUCCESS);

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(FrameLoopTest, MultipleFrameLoops) {
    // Render 3 frames to verify state machine works repeatedly
    for (int i = 0; i < 3; ++i) {
        XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        XrResult result = KinectXRRuntime::getInstance().waitFrame(session_, &waitInfo, &frameState);
        ASSERT_EQ(result, XR_SUCCESS);

        XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        result = KinectXRRuntime::getInstance().beginFrame(session_, &beginInfo);
        ASSERT_EQ(result, XR_SUCCESS);

        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = 0;
        endInfo.layers = nullptr;
        result = KinectXRRuntime::getInstance().endFrame(session_, &endInfo);
        ASSERT_EQ(result, XR_SUCCESS) << "Frame " << i << " failed";
    }
}

// M8 Tests: View Pose Queries (xrLocateViews)

TEST_F(FrameLoopTest, LocateViews_CountQuery) {
    // Create a reference space first
    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};
    spaceInfo.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};

    XrSpace space = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createReferenceSpace(session_, &spaceInfo, &space);
    ASSERT_EQ(result, XR_SUCCESS);

    // Query view count
    XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    locateInfo.displayTime = 1000;
    locateInfo.space = space;

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t viewCount = 0;

    result = KinectXRRuntime::getInstance().locateViews(session_, &locateInfo, &viewState, 0, &viewCount, nullptr);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(viewCount, 1u);  // PRIMARY_MONO has 1 view

    // Cleanup
    KinectXRRuntime::getInstance().destroySpace(space);
}

TEST_F(FrameLoopTest, LocateViews_GetViews) {
    // Create a reference space
    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};
    spaceInfo.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};

    XrSpace space = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createReferenceSpace(session_, &spaceInfo, &space);
    ASSERT_EQ(result, XR_SUCCESS);

    // Get views
    XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    locateInfo.displayTime = 1000;
    locateInfo.space = space;

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    XrView view{XR_TYPE_VIEW};
    uint32_t viewCount = 1;

    result = KinectXRRuntime::getInstance().locateViews(session_, &locateInfo, &viewState, 1, &viewCount, &view);
    EXPECT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(viewCount, 1u);

    // Verify view state flags (pose is valid and tracked)
    EXPECT_TRUE(viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT);
    EXPECT_TRUE(viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT);
    EXPECT_TRUE(viewState.viewStateFlags & XR_VIEW_STATE_POSITION_TRACKED_BIT);
    EXPECT_TRUE(viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_TRACKED_BIT);

    // Verify identity pose
    EXPECT_FLOAT_EQ(view.pose.position.x, 0.0f);
    EXPECT_FLOAT_EQ(view.pose.position.y, 0.0f);
    EXPECT_FLOAT_EQ(view.pose.position.z, 0.0f);
    EXPECT_FLOAT_EQ(view.pose.orientation.x, 0.0f);
    EXPECT_FLOAT_EQ(view.pose.orientation.y, 0.0f);
    EXPECT_FLOAT_EQ(view.pose.orientation.z, 0.0f);
    EXPECT_FLOAT_EQ(view.pose.orientation.w, 1.0f);

    // Verify FOV is reasonable (Kinect 1: 57° horizontal, 43° vertical)
    EXPECT_LT(view.fov.angleLeft, 0.0f);
    EXPECT_GT(view.fov.angleRight, 0.0f);
    EXPECT_GT(view.fov.angleUp, 0.0f);
    EXPECT_LT(view.fov.angleDown, 0.0f);

    // Cleanup
    KinectXRRuntime::getInstance().destroySpace(space);
}

TEST_F(FrameLoopTest, LocateViews_InvalidSpace) {
    XrSpace fakeSpace = reinterpret_cast<XrSpace>(0x99999);

    XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    locateInfo.displayTime = 1000;
    locateInfo.space = fakeSpace;

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t viewCount = 0;

    XrResult result = KinectXRRuntime::getInstance().locateViews(session_, &locateInfo, &viewState, 0, &viewCount, nullptr);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(FrameLoopTest, LocateViews_InvalidViewConfigType) {
    // Create a reference space
    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};
    spaceInfo.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};

    XrSpace space = XR_NULL_HANDLE;
    XrResult result = KinectXRRuntime::getInstance().createReferenceSpace(session_, &spaceInfo, &space);
    ASSERT_EQ(result, XR_SUCCESS);

    // Try with wrong view configuration type
    XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;  // Wrong type
    locateInfo.displayTime = 1000;
    locateInfo.space = space;

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t viewCount = 0;

    result = KinectXRRuntime::getInstance().locateViews(session_, &locateInfo, &viewState, 0, &viewCount, nullptr);
    EXPECT_EQ(result, XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED);

    // Cleanup
    KinectXRRuntime::getInstance().destroySpace(space);
}
