#include <gtest/gtest.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdlib>

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
