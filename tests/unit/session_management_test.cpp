#include <gtest/gtest.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdlib>
#include <vector>
#include <algorithm>

class SessionManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set XR_RUNTIME_JSON to our manifest
        std::string manifestPath = std::string(getenv("HOME")) +
            "/projects/hardware/repos/kinect-xr/manifest/kinect_xr.json";
        setenv("XR_RUNTIME_JSON", manifestPath.c_str(), 1);

        // Create instance with Metal extension
        const char* extensions[] = {"XR_KHR_metal_enable"};
        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strncpy(createInfo.applicationInfo.applicationName, "Session Management Test", XR_MAX_APPLICATION_NAME_SIZE);
        createInfo.enabledExtensionCount = 1;
        createInfo.enabledExtensionNames = extensions;

        XrResult result = xrCreateInstance(&createInfo, &instance_);
        ASSERT_EQ(result, XR_SUCCESS);
        ASSERT_NE(instance_, XR_NULL_HANDLE);

        // Get system
        XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
        getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        result = xrGetSystem(instance_, &getInfo, &systemId_);
        ASSERT_EQ(result, XR_SUCCESS);
    }

    void TearDown() override {
        if (instance_ != XR_NULL_HANDLE) {
            xrDestroyInstance(instance_);
        }
        unsetenv("XR_RUNTIME_JSON");
    }

    XrInstance instance_ = XR_NULL_HANDLE;
    XrSystemId systemId_ = XR_NULL_SYSTEM_ID;
};

TEST_F(SessionManagementTest, CreateSessionWithMetalBinding) {
    // Create dummy Metal command queue pointer (we won't actually use it in unit tests)
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    XrResult result = xrCreateSession(instance_, &sessionInfo, &session);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_NE(session, XR_NULL_HANDLE);

    // Clean up
    xrDestroySession(session);
}

TEST_F(SessionManagementTest, CreateSessionWithoutGraphicsBinding) {
    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = nullptr;  // No graphics binding
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    XrResult result = xrCreateSession(instance_, &sessionInfo, &session);

    EXPECT_EQ(result, XR_ERROR_GRAPHICS_DEVICE_INVALID);
}

TEST_F(SessionManagementTest, CreateSessionWithNullCommandQueue) {
    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = nullptr;  // Invalid

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    XrResult result = xrCreateSession(instance_, &sessionInfo, &session);

    EXPECT_EQ(result, XR_ERROR_GRAPHICS_DEVICE_INVALID);
}

TEST_F(SessionManagementTest, CreateSessionInvalidSystem) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = 999999;  // Invalid system ID

    XrSession session = XR_NULL_HANDLE;
    XrResult result = xrCreateSession(instance_, &sessionInfo, &session);

    EXPECT_EQ(result, XR_ERROR_SYSTEM_INVALID);
}

// Note: Instance validation is implicit via systemId lookup
// Invalid systems (including those from destroyed instances) return XR_ERROR_SYSTEM_INVALID

TEST_F(SessionManagementTest, CreateMultipleSessionsShouldFail) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    // Create first session
    XrSession session1 = XR_NULL_HANDLE;
    XrResult result = xrCreateSession(instance_, &sessionInfo, &session1);
    ASSERT_EQ(result, XR_SUCCESS);

    // Try to create second session - should fail (one session per instance limit)
    XrSession session2 = XR_NULL_HANDLE;
    result = xrCreateSession(instance_, &sessionInfo, &session2);
    EXPECT_EQ(result, XR_ERROR_LIMIT_REACHED);

    // Clean up
    xrDestroySession(session1);
}

TEST_F(SessionManagementTest, DestroySession) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    XrResult result = xrDestroySession(session);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(SessionManagementTest, DestroyInvalidSession) {
    XrSession invalidSession = reinterpret_cast<XrSession>(0xDEADBEEF);
    XrResult result = xrDestroySession(invalidSession);

    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(SessionManagementTest, DestroySessionTwice) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    // First destroy should succeed
    XrResult result = xrDestroySession(session);
    ASSERT_EQ(result, XR_SUCCESS);

    // Second destroy should fail
    result = xrDestroySession(session);
    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

// Session State Tests

TEST_F(SessionManagementTest, SessionStateTransitionsToReady) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    XrResult result = xrCreateSession(instance_, &sessionInfo, &session);
    ASSERT_EQ(result, XR_SUCCESS);

    // Should have READY state event
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    result = xrPollEvent(instance_, &event);
    ASSERT_EQ(result, XR_SUCCESS);

    XrEventDataSessionStateChanged* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->type, XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED);
    EXPECT_EQ(stateEvent->session, session);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_READY);

    xrDestroySession(session);
}

TEST_F(SessionManagementTest, BeginSessionTransitionsCorrectly) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    // Clear READY event
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    xrPollEvent(instance_, &event);

    // Begin session
    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;

    XrResult result = xrBeginSession(session, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // Should have SYNCHRONIZED, VISIBLE, FOCUSED events
    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    result = xrPollEvent(instance_, &event);
    ASSERT_EQ(result, XR_SUCCESS);
    XrEventDataSessionStateChanged* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_SYNCHRONIZED);

    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    result = xrPollEvent(instance_, &event);
    ASSERT_EQ(result, XR_SUCCESS);
    stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_VISIBLE);

    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    result = xrPollEvent(instance_, &event);
    ASSERT_EQ(result, XR_SUCCESS);
    stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_FOCUSED);

    xrEndSession(session);
    xrDestroySession(session);
}

TEST_F(SessionManagementTest, EndSessionTransitionsCorrectly) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    // Clear READY event
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    xrPollEvent(instance_, &event);

    // Begin session
    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    xrBeginSession(session, &beginInfo);

    // Clear begin events
    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    while (xrPollEvent(instance_, &event) == XR_SUCCESS) {
        event.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    // End session
    XrResult result = xrEndSession(session);
    ASSERT_EQ(result, XR_SUCCESS);

    // Should have STOPPING, IDLE events
    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    result = xrPollEvent(instance_, &event);
    ASSERT_EQ(result, XR_SUCCESS);
    XrEventDataSessionStateChanged* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_STOPPING);

    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    result = xrPollEvent(instance_, &event);
    ASSERT_EQ(result, XR_SUCCESS);
    stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_IDLE);

    xrDestroySession(session);
}

TEST_F(SessionManagementTest, PollEventReturnsUnavailableWhenEmpty) {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    XrResult result = xrPollEvent(instance_, &event);

    EXPECT_EQ(result, XR_EVENT_UNAVAILABLE);
}

TEST_F(SessionManagementTest, BeginSessionUnsupportedViewConfig) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    // Clear READY event
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    xrPollEvent(instance_, &event);

    // Try to begin with unsupported view config
    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    XrResult result = xrBeginSession(session, &beginInfo);
    EXPECT_EQ(result, XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED);

    xrDestroySession(session);
}

// Reference Space Tests

TEST_F(SessionManagementTest, EnumerateReferenceSpaces) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    // Get count
    uint32_t spaceCount = 0;
    XrResult result = xrEnumerateReferenceSpaces(session, 0, &spaceCount, nullptr);
    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(spaceCount, 3u);  // VIEW, LOCAL, STAGE

    // Get types
    std::vector<XrReferenceSpaceType> spaceTypes(spaceCount);
    result = xrEnumerateReferenceSpaces(session, spaceCount, &spaceCount, spaceTypes.data());
    ASSERT_EQ(result, XR_SUCCESS);

    // Verify we have VIEW, LOCAL, and STAGE
    EXPECT_TRUE(std::find(spaceTypes.begin(), spaceTypes.end(), XR_REFERENCE_SPACE_TYPE_VIEW) != spaceTypes.end());
    EXPECT_TRUE(std::find(spaceTypes.begin(), spaceTypes.end(), XR_REFERENCE_SPACE_TYPE_LOCAL) != spaceTypes.end());
    EXPECT_TRUE(std::find(spaceTypes.begin(), spaceTypes.end(), XR_REFERENCE_SPACE_TYPE_STAGE) != spaceTypes.end());

    xrDestroySession(session);
}

TEST_F(SessionManagementTest, CreateReferenceSpaceView) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};  // Identity

    XrSpace space = XR_NULL_HANDLE;
    XrResult result = xrCreateReferenceSpace(session, &spaceInfo, &space);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_NE(space, XR_NULL_HANDLE);

    xrDestroySpace(space);
    xrDestroySession(session);
}

TEST_F(SessionManagementTest, CreateReferenceSpaceLocal) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};

    XrSpace space = XR_NULL_HANDLE;
    XrResult result = xrCreateReferenceSpace(session, &spaceInfo, &space);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_NE(space, XR_NULL_HANDLE);

    xrDestroySpace(space);
    xrDestroySession(session);
}

TEST_F(SessionManagementTest, CreateReferenceSpaceStage) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};

    XrSpace space = XR_NULL_HANDLE;
    XrResult result = xrCreateReferenceSpace(session, &spaceInfo, &space);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_NE(space, XR_NULL_HANDLE);

    xrDestroySpace(space);
    xrDestroySession(session);
}

TEST_F(SessionManagementTest, CreateReferenceSpaceUnsupported) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;  // Unsupported
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};

    XrSpace space = XR_NULL_HANDLE;
    XrResult result = xrCreateReferenceSpace(session, &spaceInfo, &space);

    EXPECT_EQ(result, XR_ERROR_REFERENCE_SPACE_UNSUPPORTED);

    xrDestroySession(session);
}

TEST_F(SessionManagementTest, DestroySpace) {
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);

    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId_;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance_, &sessionInfo, &session);

    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};

    XrSpace space = XR_NULL_HANDLE;
    xrCreateReferenceSpace(session, &spaceInfo, &space);

    XrResult result = xrDestroySpace(space);
    EXPECT_EQ(result, XR_SUCCESS);

    xrDestroySession(session);
}

TEST_F(SessionManagementTest, DestroyInvalidSpace) {
    XrSpace invalidSpace = reinterpret_cast<XrSpace>(0xDEADBEEF);
    XrResult result = xrDestroySpace(invalidSpace);

    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

// Graphics Requirements Test

TEST_F(SessionManagementTest, GetMetalGraphicsRequirements) {
    // Get function pointer via xrGetInstanceProcAddr
    PFN_xrGetMetalGraphicsRequirementsKHR pfnGetMetalGraphicsRequirements = nullptr;
    XrResult result = xrGetInstanceProcAddr(instance_, "xrGetMetalGraphicsRequirementsKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetMetalGraphicsRequirements));
    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_NE(pfnGetMetalGraphicsRequirements, nullptr);

    XrGraphicsRequirementsMetalKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_METAL_KHR};
    result = pfnGetMetalGraphicsRequirements(instance_, systemId_, &requirements);

    ASSERT_EQ(result, XR_SUCCESS);
    // nullptr indicates any Metal device is acceptable
    EXPECT_EQ(requirements.metalDevice, nullptr);
}
