#include "kinect_xr/runtime.h"
#include <openxr/openxr_platform.h>
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
        // Supported extensions
        if (strcmp(extName, "XR_KHR_composition_layer_depth") != 0 &&
            strcmp(extName, "XR_KHR_metal_enable") != 0) {
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

XrResult KinectXRRuntime::createSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session) {
    if (!createInfo || !session) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (createInfo->type != XR_TYPE_SESSION_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate instance and system
    {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        auto it = instances_.find(instance);
        if (it == instances_.end()) {
            return XR_ERROR_HANDLE_INVALID;
        }

        if (!it->second->system || it->second->system->systemId != createInfo->systemId) {
            return XR_ERROR_SYSTEM_INVALID;
        }
    }

    // Validate graphics binding - must have Metal binding in next chain
    const XrGraphicsBindingMetalKHR* metalBinding = nullptr;
    const void* nextPtr = createInfo->next;
    while (nextPtr != nullptr) {
        const XrBaseInStructure* base = reinterpret_cast<const XrBaseInStructure*>(nextPtr);
        if (base->type == XR_TYPE_GRAPHICS_BINDING_METAL_KHR) {
            metalBinding = reinterpret_cast<const XrGraphicsBindingMetalKHR*>(nextPtr);
            break;
        }
        nextPtr = base->next;
    }

    if (!metalBinding) {
        return XR_ERROR_GRAPHICS_DEVICE_INVALID;
    }

    if (!metalBinding->commandQueue) {
        return XR_ERROR_GRAPHICS_DEVICE_INVALID;
    }

    // Only allow one session per instance
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        for (const auto& [handle, sessionData] : sessions_) {
            if (sessionData->instance == instance) {
                return XR_ERROR_LIMIT_REACHED;
            }
        }
    }

    // Create session handle
    std::lock_guard<std::mutex> lock(sessionMutex_);
    XrSession handle = reinterpret_cast<XrSession>(nextSessionId_++);

    auto sessionData = std::make_unique<SessionData>(handle, instance, createInfo->systemId);
    sessionData->metalCommandQueue = metalBinding->commandQueue;

    sessions_[handle] = std::move(sessionData);
    *session = handle;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::destroySession(XrSession session) {
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Session must be in IDLE or STOPPING state to destroy
    if (it->second->state != SessionState::IDLE && it->second->state != SessionState::STOPPING) {
        return XR_ERROR_SESSION_RUNNING;
    }

    sessions_.erase(it);
    return XR_SUCCESS;
}

bool KinectXRRuntime::isValidSession(XrSession session) const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    return sessions_.find(session) != sessions_.end();
}

SessionData* KinectXRRuntime::getSessionData(XrSession session) {
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return nullptr;
    }

    return it->second.get();
}

} // namespace kinect_xr
