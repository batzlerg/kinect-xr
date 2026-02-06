#include <gtest/gtest.h>
#include <openxr/openxr.h>
#include <cstdlib>
#include <vector>

class SystemManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set XR_RUNTIME_JSON to our manifest
        std::string manifestPath = std::string(getenv("HOME")) +
            "/projects/hardware/repos/kinect-xr/manifest/kinect_xr.json";
        setenv("XR_RUNTIME_JSON", manifestPath.c_str(), 1);

        // Create instance
        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strncpy(createInfo.applicationInfo.applicationName, "System Management Test", XR_MAX_APPLICATION_NAME_SIZE);

        XrResult result = xrCreateInstance(&createInfo, &instance_);
        ASSERT_EQ(result, XR_SUCCESS);
        ASSERT_NE(instance_, XR_NULL_HANDLE);
    }

    void TearDown() override {
        if (instance_ != XR_NULL_HANDLE) {
            xrDestroyInstance(instance_);
        }
        unsetenv("XR_RUNTIME_JSON");
    }

    XrInstance instance_ = XR_NULL_HANDLE;
};

TEST_F(SystemManagementTest, GetSystemHeadMountedDisplay) {
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrResult result = xrGetSystem(instance_, &getInfo, &systemId);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_NE(systemId, XR_NULL_SYSTEM_ID);
}

TEST_F(SystemManagementTest, GetSystemUnsupportedFormFactor) {
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HANDHELD_DISPLAY;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrResult result = xrGetSystem(instance_, &getInfo, &systemId);

    EXPECT_EQ(result, XR_ERROR_FORM_FACTOR_UNSUPPORTED);
}

TEST_F(SystemManagementTest, GetSystemInvalidInstance) {
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrInstance invalidInstance = reinterpret_cast<XrInstance>(0xDEADBEEF);
    XrResult result = xrGetSystem(invalidInstance, &getInfo, &systemId);

    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}

TEST_F(SystemManagementTest, GetSystemNullPointers) {
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;

    // Null getInfo
    XrResult result = xrGetSystem(instance_, nullptr, &systemId);
    EXPECT_EQ(result, XR_ERROR_VALIDATION_FAILURE);

    // Null systemId
    result = xrGetSystem(instance_, &getInfo, nullptr);
    EXPECT_EQ(result, XR_ERROR_VALIDATION_FAILURE);
}

TEST_F(SystemManagementTest, GetSystemProperties) {
    // First get system
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrResult result = xrGetSystem(instance_, &getInfo, &systemId);
    ASSERT_EQ(result, XR_SUCCESS);

    // Get properties
    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    result = xrGetSystemProperties(instance_, systemId, &properties);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(properties.systemId, systemId);
    EXPECT_EQ(properties.vendorId, 0x045e);  // Microsoft
    EXPECT_STREQ(properties.systemName, "Kinect XR System");
    EXPECT_EQ(properties.graphicsProperties.maxSwapchainImageWidth, 640u);
    EXPECT_EQ(properties.graphicsProperties.maxSwapchainImageHeight, 480u);
    EXPECT_EQ(properties.graphicsProperties.maxLayerCount, 1u);
    EXPECT_EQ(properties.trackingProperties.orientationTracking, XR_FALSE);
    EXPECT_EQ(properties.trackingProperties.positionTracking, XR_FALSE);
}

TEST_F(SystemManagementTest, GetSystemPropertiesInvalidSystem) {
    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    XrSystemId invalidSystemId = 999999;
    XrResult result = xrGetSystemProperties(instance_, invalidSystemId, &properties);

    EXPECT_EQ(result, XR_ERROR_SYSTEM_INVALID);
}

TEST_F(SystemManagementTest, GetSystemPropertiesNullProperties) {
    // First get system
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrResult result = xrGetSystem(instance_, &getInfo, &systemId);
    ASSERT_EQ(result, XR_SUCCESS);

    // Null properties
    result = xrGetSystemProperties(instance_, systemId, nullptr);
    EXPECT_EQ(result, XR_ERROR_VALIDATION_FAILURE);
}

TEST_F(SystemManagementTest, GetSystemIdempotent) {
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId systemId1 = XR_NULL_SYSTEM_ID;
    XrResult result = xrGetSystem(instance_, &getInfo, &systemId1);
    ASSERT_EQ(result, XR_SUCCESS);

    // Call again - should return same system ID
    XrSystemId systemId2 = XR_NULL_SYSTEM_ID;
    result = xrGetSystem(instance_, &getInfo, &systemId2);
    ASSERT_EQ(result, XR_SUCCESS);

    EXPECT_EQ(systemId1, systemId2);
}

// View Configuration Tests

TEST_F(SystemManagementTest, EnumerateViewConfigurationTypes) {
    // Get system first
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance_, &getInfo, &systemId);

    // Get count
    uint32_t viewConfigCount = 0;
    XrResult result = xrEnumerateViewConfigurations(instance_, systemId, 0, &viewConfigCount, nullptr);
    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(viewConfigCount, 1u);

    // Get types
    std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigCount);
    result = xrEnumerateViewConfigurations(instance_, systemId, viewConfigCount, &viewConfigCount, viewConfigTypes.data());
    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(viewConfigTypes[0], XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO);
}

TEST_F(SystemManagementTest, GetViewConfigurationProperties) {
    // Get system first
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance_, &getInfo, &systemId);

    XrViewConfigurationProperties props{XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
    XrResult result = xrGetViewConfigurationProperties(instance_, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, &props);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(props.viewConfigurationType, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO);
    EXPECT_EQ(props.fovMutable, XR_FALSE);
}

TEST_F(SystemManagementTest, GetViewConfigurationPropertiesUnsupportedType) {
    // Get system first
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance_, &getInfo, &systemId);

    XrViewConfigurationProperties props{XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
    XrResult result = xrGetViewConfigurationProperties(instance_, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, &props);

    EXPECT_EQ(result, XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED);
}

TEST_F(SystemManagementTest, EnumerateViewConfigurationViews) {
    // Get system first
    XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance_, &getInfo, &systemId);

    // Get count
    uint32_t viewCount = 0;
    XrResult result = xrEnumerateViewConfigurationViews(instance_, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, 0, &viewCount, nullptr);
    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(viewCount, 1u);

    // Get views
    std::vector<XrViewConfigurationView> views(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    result = xrEnumerateViewConfigurationViews(instance_, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO, viewCount, &viewCount, views.data());
    ASSERT_EQ(result, XR_SUCCESS);

    // Verify Kinect 1 RGB specs
    EXPECT_EQ(views[0].recommendedImageRectWidth, 640u);
    EXPECT_EQ(views[0].maxImageRectWidth, 640u);
    EXPECT_EQ(views[0].recommendedImageRectHeight, 480u);
    EXPECT_EQ(views[0].maxImageRectHeight, 480u);
    EXPECT_EQ(views[0].recommendedSwapchainSampleCount, 1u);
    EXPECT_EQ(views[0].maxSwapchainSampleCount, 1u);
}
