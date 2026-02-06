#include <gtest/gtest.h>
#include <openxr/openxr.h>
#include <cstdlib>
#include <string>
#include <vector>

class RuntimeDiscoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set XR_RUNTIME_JSON to our manifest
        std::string manifestPath = std::string(getenv("HOME")) +
            "/projects/hardware/repos/kinect-xr/manifest/kinect_xr.json";
        setenv("XR_RUNTIME_JSON", manifestPath.c_str(), 1);
    }

    void TearDown() override {
        // Clean up environment variable
        unsetenv("XR_RUNTIME_JSON");
    }
};

TEST_F(RuntimeDiscoveryTest, EnumerateExtensions) {
    // First call: get count
    uint32_t extensionCount = 0;
    XrResult result = xrEnumerateInstanceExtensionProperties(
        nullptr, 0, &extensionCount, nullptr);

    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_GT(extensionCount, 0u) << "Runtime should support at least one extension";

    // Second call: get properties
    std::vector<XrExtensionProperties> extensions(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    result = xrEnumerateInstanceExtensionProperties(
        nullptr, extensionCount, &extensionCount, extensions.data());

    ASSERT_EQ(result, XR_SUCCESS);

    // Verify our extension is present
    bool foundDepthExtension = false;
    for (const auto& ext : extensions) {
        if (std::string(ext.extensionName) == "XR_KHR_composition_layer_depth") {
            foundDepthExtension = true;
            break;
        }
    }

    EXPECT_TRUE(foundDepthExtension)
        << "Expected to find XR_KHR_composition_layer_depth extension";
}

TEST_F(RuntimeDiscoveryTest, EnumerateApiLayers) {
    // Should return 0 layers
    uint32_t layerCount = 0;
    XrResult result = xrEnumerateApiLayerProperties(0, &layerCount, nullptr);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_EQ(layerCount, 0u) << "Runtime should not support any API layers";
}

TEST_F(RuntimeDiscoveryTest, CreateAndDestroyInstance) {
    // Create instance
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strncpy(createInfo.applicationInfo.applicationName, "Runtime Discovery Test", XR_MAX_APPLICATION_NAME_SIZE);

    XrInstance instance = XR_NULL_HANDLE;
    XrResult result = xrCreateInstance(&createInfo, &instance);

    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_NE(instance, XR_NULL_HANDLE);

    // Get instance properties
    XrInstanceProperties instanceProps{XR_TYPE_INSTANCE_PROPERTIES};
    result = xrGetInstanceProperties(instance, &instanceProps);

    ASSERT_EQ(result, XR_SUCCESS);
    EXPECT_STREQ(instanceProps.runtimeName, "Kinect XR Runtime");

    // Destroy instance
    result = xrDestroyInstance(instance);
    EXPECT_EQ(result, XR_SUCCESS);
}

TEST_F(RuntimeDiscoveryTest, CreateInstanceWithExtension) {
    // Create instance with our supported extension
    const char* extensions[] = {"XR_KHR_composition_layer_depth"};

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strncpy(createInfo.applicationInfo.applicationName, "Extension Test", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = extensions;

    XrInstance instance = XR_NULL_HANDLE;
    XrResult result = xrCreateInstance(&createInfo, &instance);

    ASSERT_EQ(result, XR_SUCCESS);
    ASSERT_NE(instance, XR_NULL_HANDLE);

    // Clean up
    xrDestroyInstance(instance);
}

TEST_F(RuntimeDiscoveryTest, CreateInstanceWithUnsupportedExtension) {
    // Try to create instance with unsupported extension
    const char* extensions[] = {"XR_KHR_unsupported_extension"};

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    strncpy(createInfo.applicationInfo.applicationName, "Unsupported Extension Test", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = extensions;

    XrInstance instance = XR_NULL_HANDLE;
    XrResult result = xrCreateInstance(&createInfo, &instance);

    EXPECT_EQ(result, XR_ERROR_EXTENSION_NOT_PRESENT);
}

TEST_F(RuntimeDiscoveryTest, DestroyInvalidInstance) {
    // Try to destroy invalid instance handle
    XrInstance invalidInstance = reinterpret_cast<XrInstance>(0xDEADBEEF);
    XrResult result = xrDestroyInstance(invalidInstance);

    EXPECT_EQ(result, XR_ERROR_HANDLE_INVALID);
}
