#include <gtest/gtest.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdlib>

class SessionLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set XR_RUNTIME_JSON to our manifest
        std::string manifestPath = std::string(getenv("HOME")) +
            "/projects/hardware/repos/kinect-xr/manifest/kinect_xr.json";
        setenv("XR_RUNTIME_JSON", manifestPath.c_str(), 1);
    }

    void TearDown() override {
        unsetenv("XR_RUNTIME_JSON");
    }
};

TEST_F(SessionLifecycleTest, FullSessionLifecycle) {
    // 1. Create instance
    const char* extensions[] = {"XR_KHR_metal_enable"};
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strncpy(createInfo.applicationInfo.applicationName, "Session Lifecycle Test", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = extensions;

    XrInstance instance = XR_NULL_HANDLE;
    XrResult result = xrCreateInstance(&createInfo, &instance);
    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_NE(instance, XR_NULL_HANDLE);

    // 2. Get system
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    result = xrGetSystem(instance, &getInfo, &systemId);
    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_NE(systemId, XR_NULL_SYSTEM_ID);

    // 3. Get graphics requirements
    PFN_xrGetMetalGraphicsRequirementsKHR pfnGetMetalGraphicsRequirements = nullptr;
    result = xrGetInstanceProcAddr(instance, "xrGetMetalGraphicsRequirementsKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetMetalGraphicsRequirements));
    ASSERT_EQ(result, XR_SUCCESS);

    XrGraphicsRequirementsMetalKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_METAL_KHR};
    result = pfnGetMetalGraphicsRequirements(instance, systemId, &requirements);
    ASSERT_EQ(result, XR_SUCCESS);

    // 4. Create session
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);
    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId;

    XrSession session = XR_NULL_HANDLE;
    result = xrCreateSession(instance, &sessionInfo, &session);
    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_NE(session, XR_NULL_HANDLE);

    // 5. Poll for READY event
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    result = xrPollEvent(instance, &event);
    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(event.type, XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED);

    XrEventDataSessionStateChanged* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_READY);

    // 6. Begin session
    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
    result = xrBeginSession(session, &beginInfo);
    ASSERT_EQ(result, XR_SUCCESS);

    // Poll state change events
    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    while (xrPollEvent(instance, &event) == XR_SUCCESS) {
        event.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    // 7. End session
    result = xrEndSession(session);
    ASSERT_EQ(result, XR_SUCCESS);

    // Poll state change events
    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    while (xrPollEvent(instance, &event) == XR_SUCCESS) {
        event.type = XR_TYPE_EVENT_DATA_BUFFER;
    }

    // 8. Destroy session
    result = xrDestroySession(session);
    EXPECT_EQ(result, XR_SUCCESS);

    // 9. Destroy instance
    result = xrDestroyInstance(instance);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(SessionLifecycleTest, ReferenceSpaceCreation) {
    // Create instance
    const char* extensions[] = {"XR_KHR_metal_enable"};
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strncpy(createInfo.applicationInfo.applicationName, "Reference Space Test", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = extensions;

    XrInstance instance = XR_NULL_HANDLE;
    xrCreateInstance(&createInfo, &instance);

    // Get system
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance, &getInfo, &systemId);

    // Create session
    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);
    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance, &sessionInfo, &session);

    // Enumerate reference spaces
    uint32_t spaceCount = 0;
    XrResult result = xrEnumerateReferenceSpaces(session, 0, &spaceCount, nullptr);
    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(spaceCount, 3u);

    // Create VIEW space
    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};

    XrSpace space = XR_NULL_HANDLE;
    result = xrCreateReferenceSpace(session, &spaceInfo, &space);
    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_NE(space, XR_NULL_HANDLE);

    // Cleanup
    xrDestroySpace(space);
    xrDestroySession(session);
    xrDestroyInstance(instance);
}

TEST_F(SessionLifecycleTest, StateEventPolling) {
    // Create instance and session
    const char* extensions[] = {"XR_KHR_metal_enable"};
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strncpy(createInfo.applicationInfo.applicationName, "State Event Test", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = extensions;

    XrInstance instance = XR_NULL_HANDLE;
    xrCreateInstance(&createInfo, &instance);

    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance, &getInfo, &systemId);

    void* dummyCommandQueue = reinterpret_cast<void*>(0x12345678);
    XrGraphicsBindingMetalKHR metalBinding{XR_TYPE_GRAPHICS_BINDING_METAL_KHR};
    metalBinding.commandQueue = dummyCommandQueue;

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &metalBinding;
    sessionInfo.systemId = systemId;

    XrSession session = XR_NULL_HANDLE;
    xrCreateSession(instance, &sessionInfo, &session);

    // Poll READY event
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    XrResult result = xrPollEvent(instance, &event);
    ASSERT_EQ(result, XR_SUCCESS);

    XrEventDataSessionStateChanged* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
    EXPECT_EQ(stateEvent->type, XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED);
    EXPECT_EQ(stateEvent->state, XR_SESSION_STATE_READY);
    EXPECT_EQ(stateEvent->session, session);

    // No more events
    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    result = xrPollEvent(instance, &event);
    EXPECT_EQ(result, XR_EVENT_UNAVAILABLE);

    // Cleanup
    xrDestroySession(session);
    xrDestroyInstance(instance);
}
