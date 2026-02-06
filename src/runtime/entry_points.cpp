#include "kinect_xr/runtime.h"
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <cstring>

// Forward declarations of entry points
extern "C" {

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(
    const char* layerName,
    uint32_t propertyCapacityInput,
    uint32_t* propertyCountOutput,
    XrExtensionProperties* properties);

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties(
    uint32_t propertyCapacityInput,
    uint32_t* propertyCountOutput,
    XrApiLayerProperties* properties);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance(
    const XrInstanceCreateInfo* createInfo,
    XrInstance* instance);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance(
    XrInstance instance);

XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties(
    XrInstance instance,
    XrInstanceProperties* instanceProperties);

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystem(
    XrInstance instance,
    const XrSystemGetInfo* getInfo,
    XrSystemId* systemId);

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemProperties(
    XrInstance instance,
    XrSystemId systemId,
    XrSystemProperties* properties);

// Main entry point for the runtime
XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr(
    XrInstance instance,
    const char* name,
    PFN_xrVoidFunction* function) {

    if (!name || !function) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Instance-agnostic functions (can be called with XR_NULL_HANDLE)
    if (strcmp(name, "xrEnumerateInstanceExtensionProperties") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrEnumerateInstanceExtensionProperties);
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrEnumerateApiLayerProperties") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrEnumerateApiLayerProperties);
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrCreateInstance") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrCreateInstance);
        return XR_SUCCESS;
    }

    // Instance-specific functions (require valid instance handle)
    if (instance == XR_NULL_HANDLE) {
        // These functions require a valid instance
        *function = nullptr;
        return XR_ERROR_HANDLE_INVALID;
    }

    // Validate instance handle
    if (!kinect_xr::KinectXRRuntime::getInstance().isValidInstance(instance)) {
        *function = nullptr;
        return XR_ERROR_HANDLE_INVALID;
    }

    if (strcmp(name, "xrDestroyInstance") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrDestroyInstance);
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetInstanceProperties") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrGetInstanceProperties);
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetInstanceProcAddr") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrGetInstanceProcAddr);
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetSystem") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrGetSystem);
        return XR_SUCCESS;
    }
    if (strcmp(name, "xrGetSystemProperties") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(xrGetSystemProperties);
        return XR_SUCCESS;
    }

    // Function not supported
    *function = nullptr;
    return XR_ERROR_FUNCTION_UNSUPPORTED;
}

// Export xrGetInstanceProcAddr as the main entry point
// The loader will use this to discover all other functions
#if defined(__APPLE__)
    __attribute__((visibility("default")))
#elif defined(_WIN32)
    __declspec(dllexport)
#endif
XRAPI_ATTR XrResult XRAPI_CALL xrNegotiateLoaderRuntimeInterface(
    const XrNegotiateLoaderInfo* loaderInfo,
    XrNegotiateRuntimeRequest* runtimeRequest) {

    if (!loaderInfo || !runtimeRequest) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
        loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo)) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (runtimeRequest->structType != XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST ||
        runtimeRequest->structVersion != XR_RUNTIME_INFO_STRUCT_VERSION ||
        runtimeRequest->structSize != sizeof(XrNegotiateRuntimeRequest)) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Check version compatibility
    if (loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_RUNTIME_VERSION ||
        loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_RUNTIME_VERSION) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    runtimeRequest->runtimeInterfaceVersion = XR_CURRENT_LOADER_RUNTIME_VERSION;
    runtimeRequest->runtimeApiVersion = XR_CURRENT_API_VERSION;
    runtimeRequest->getInstanceProcAddr = xrGetInstanceProcAddr;

    return XR_SUCCESS;
}

// Implementation of enumeration functions

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(
    const char* layerName,
    uint32_t propertyCapacityInput,
    uint32_t* propertyCountOutput,
    XrExtensionProperties* properties) {

    if (!propertyCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // We don't support API layers, so reject if a layer is specified
    if (layerName != nullptr) {
        return XR_ERROR_API_LAYER_NOT_PRESENT;
    }

    // List of supported extensions
    static const char* supportedExtensions[] = {
        "XR_KHR_composition_layer_depth"
    };
    static const uint32_t extensionCount = sizeof(supportedExtensions) / sizeof(supportedExtensions[0]);

    // Two-call idiom: first call gets count, second call fills buffer
    if (propertyCapacityInput == 0) {
        // First call: return count
        *propertyCountOutput = extensionCount;
        return XR_SUCCESS;
    }

    // Second call: fill buffer
    if (propertyCapacityInput < extensionCount) {
        *propertyCountOutput = extensionCount;
        return XR_ERROR_SIZE_INSUFFICIENT;
    }

    if (!properties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Fill in extension properties
    for (uint32_t i = 0; i < extensionCount; ++i) {
        if (properties[i].type != XR_TYPE_EXTENSION_PROPERTIES) {
            return XR_ERROR_VALIDATION_FAILURE;
        }
        strncpy(properties[i].extensionName, supportedExtensions[i], XR_MAX_EXTENSION_NAME_SIZE - 1);
        properties[i].extensionName[XR_MAX_EXTENSION_NAME_SIZE - 1] = '\0';
        properties[i].extensionVersion = 1;
    }

    *propertyCountOutput = extensionCount;
    return XR_SUCCESS;
}

XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties(
    uint32_t propertyCapacityInput,
    uint32_t* propertyCountOutput,
    XrApiLayerProperties* properties) {

    if (!propertyCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // We don't support any API layers
    *propertyCountOutput = 0;

    // If caller allocated space for properties, that's fine, we just return 0
    return XR_SUCCESS;
}

// Implementation of instance lifecycle functions

XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance(
    const XrInstanceCreateInfo* createInfo,
    XrInstance* instance) {

    return kinect_xr::KinectXRRuntime::getInstance().createInstance(createInfo, instance);
}

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance(
    XrInstance instance) {

    return kinect_xr::KinectXRRuntime::getInstance().destroyInstance(instance);
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties(
    XrInstance instance,
    XrInstanceProperties* instanceProperties) {

    if (!instanceProperties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (instanceProperties->type != XR_TYPE_INSTANCE_PROPERTIES) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (!kinect_xr::KinectXRRuntime::getInstance().isValidInstance(instance)) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Fill in runtime properties
    instanceProperties->runtimeVersion = XR_MAKE_VERSION(0, 1, 0);
    strncpy(instanceProperties->runtimeName, "Kinect XR Runtime", XR_MAX_RUNTIME_NAME_SIZE - 1);
    instanceProperties->runtimeName[XR_MAX_RUNTIME_NAME_SIZE - 1] = '\0';

    return XR_SUCCESS;
}

// System management functions

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystem(
    XrInstance instance,
    const XrSystemGetInfo* getInfo,
    XrSystemId* systemId) {

    return kinect_xr::KinectXRRuntime::getInstance().getSystem(instance, getInfo, systemId);
}

XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemProperties(
    XrInstance instance,
    XrSystemId systemId,
    XrSystemProperties* properties) {

    return kinect_xr::KinectXRRuntime::getInstance().getSystemProperties(instance, systemId, properties);
}

} // extern "C"
