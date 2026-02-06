#include "kinect_xr/runtime.h"
#include <cstring>

namespace kinect_xr {

KinectXRRuntime& KinectXRRuntime::getInstance() {
    static KinectXRRuntime instance;
    return instance;
}

XrResult KinectXRRuntime::createInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance) {
    if (!createInfo || !instance) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate structure type
    if (createInfo->type != XR_TYPE_INSTANCE_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate API version
    if (XR_VERSION_MAJOR(createInfo->applicationInfo.apiVersion) > XR_VERSION_MAJOR(XR_CURRENT_API_VERSION)) {
        return XR_ERROR_API_VERSION_UNSUPPORTED;
    }

    // Check requested extensions
    for (uint32_t i = 0; i < createInfo->enabledExtensionCount; ++i) {
        const char* extName = createInfo->enabledExtensionNames[i];
        // Currently we only support XR_KHR_composition_layer_depth
        if (strcmp(extName, "XR_KHR_composition_layer_depth") != 0) {
            return XR_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    // Create instance handle (use pointer to InstanceData as handle)
    std::lock_guard<std::mutex> lock(instanceMutex_);

    // Generate unique handle
    XrInstance handle = reinterpret_cast<XrInstance>(nextInstanceId_++);

    // Create instance data
    auto instanceData = std::make_unique<InstanceData>(handle);
    instanceData->applicationName = createInfo->applicationInfo.applicationName;
    instanceData->applicationVersion = createInfo->applicationInfo.applicationVersion;
    instanceData->engineName = createInfo->applicationInfo.engineName;
    instanceData->engineVersion = createInfo->applicationInfo.engineVersion;
    instanceData->apiVersion = createInfo->applicationInfo.apiVersion;

    instances_[handle] = std::move(instanceData);
    *instance = handle;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::destroyInstance(XrInstance instance) {
    std::lock_guard<std::mutex> lock(instanceMutex_);

    auto it = instances_.find(instance);
    if (it == instances_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    instances_.erase(it);
    return XR_SUCCESS;
}

bool KinectXRRuntime::isValidInstance(XrInstance instance) const {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    return instances_.find(instance) != instances_.end();
}

InstanceData* KinectXRRuntime::getInstanceData(XrInstance instance) {
    std::lock_guard<std::mutex> lock(instanceMutex_);

    auto it = instances_.find(instance);
    if (it == instances_.end()) {
        return nullptr;
    }

    return it->second.get();
}

XrResult KinectXRRuntime::getSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId) {
    if (!getInfo || !systemId) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (getInfo->type != XR_TYPE_SYSTEM_GET_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(instanceMutex_);

    auto it = instances_.find(instance);
    if (it == instances_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Only support HEAD_MOUNTED_DISPLAY form factor
    if (getInfo->formFactor != XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) {
        return XR_ERROR_FORM_FACTOR_UNSUPPORTED;
    }

    // Create system if it doesn't exist for this instance
    if (!it->second->system) {
        XrSystemId sysId = static_cast<XrSystemId>(nextSystemId_++);
        it->second->system = std::make_unique<SystemData>(sysId);
    }

    *systemId = it->second->system->systemId;
    return XR_SUCCESS;
}

XrResult KinectXRRuntime::getSystemProperties(XrInstance instance, XrSystemId systemId, XrSystemProperties* properties) {
    if (!properties) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (properties->type != XR_TYPE_SYSTEM_PROPERTIES) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(instanceMutex_);

    auto it = instances_.find(instance);
    if (it == instances_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    if (!it->second->system || it->second->system->systemId != systemId) {
        return XR_ERROR_SYSTEM_INVALID;
    }

    // Fill in system properties
    properties->systemId = systemId;
    properties->vendorId = 0x045e;  // Microsoft vendor ID (Kinect manufacturer)
    strncpy(properties->systemName, "Kinect XR System", XR_MAX_SYSTEM_NAME_SIZE - 1);
    properties->systemName[XR_MAX_SYSTEM_NAME_SIZE - 1] = '\0';

    // Kinect 1 specifications
    properties->graphicsProperties.maxSwapchainImageWidth = 640;
    properties->graphicsProperties.maxSwapchainImageHeight = 480;
    properties->graphicsProperties.maxLayerCount = 1;

    // Tracking properties - Kinect is a stationary sensor
    properties->trackingProperties.orientationTracking = XR_FALSE;
    properties->trackingProperties.positionTracking = XR_FALSE;

    return XR_SUCCESS;
}

bool KinectXRRuntime::isValidSystem(XrInstance instance, XrSystemId systemId) const {
    std::lock_guard<std::mutex> lock(instanceMutex_);

    auto it = instances_.find(instance);
    if (it == instances_.end()) {
        return false;
    }

    return it->second->system && it->second->system->systemId == systemId;
}

} // namespace kinect_xr
