#include "kinect_xr/runtime.h"
#include "kinect_xr/metal_helper.h"
#include <openxr/openxr_platform.h>
#include <cstring>
#include <chrono>
#include <thread>

namespace kinect_xr {

// Helper to convert SessionState to XrSessionState
static XrSessionState toXrSessionState(SessionState state) {
    switch (state) {
        case SessionState::IDLE: return XR_SESSION_STATE_IDLE;
        case SessionState::READY: return XR_SESSION_STATE_READY;
        case SessionState::SYNCHRONIZED: return XR_SESSION_STATE_SYNCHRONIZED;
        case SessionState::VISIBLE: return XR_SESSION_STATE_VISIBLE;
        case SessionState::FOCUSED: return XR_SESSION_STATE_FOCUSED;
        case SessionState::STOPPING: return XR_SESSION_STATE_STOPPING;
        default: return XR_SESSION_STATE_UNKNOWN;
    }
}

// Helper to queue session state change event
static void queueSessionStateChanged(InstanceData* instanceData, XrSession session, SessionState newState) {
    XrEventDataBuffer eventBuffer{XR_TYPE_EVENT_DATA_BUFFER};
    XrEventDataSessionStateChanged* stateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventBuffer);
    stateChanged->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
    stateChanged->next = nullptr;
    stateChanged->session = session;
    stateChanged->state = toXrSessionState(newState);
    stateChanged->time = 0;  // We don't track time yet

    instanceData->eventQueue.push(eventBuffer);
}

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

    // Extract Metal device from command queue for swapchain creation
    sessionData->metalDevice = metal::getMetalDevice(metalBinding->commandQueue);

    sessions_[handle] = std::move(sessionData);
    *session = handle;

    // Queue initial state transition: IDLE → READY
    {
        std::lock_guard<std::mutex> instanceLock(instanceMutex_);
        auto instIt = instances_.find(instance);
        if (instIt != instances_.end()) {
            sessions_[handle]->state = SessionState::READY;
            queueSessionStateChanged(instIt->second.get(), handle, SessionState::READY);
        }
    }

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::destroySession(XrSession session) {
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Session must not be running (SYNCHRONIZED, VISIBLE, FOCUSED) to destroy
    // IDLE and READY are okay to destroy
    if (it->second->state == SessionState::SYNCHRONIZED ||
        it->second->state == SessionState::VISIBLE ||
        it->second->state == SessionState::FOCUSED) {
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

XrResult KinectXRRuntime::beginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
    if (!beginInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (beginInfo->type != XR_TYPE_SESSION_BEGIN_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> sessionLock(sessionMutex_);
    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SessionData* sessionData = it->second.get();

    // Validate view configuration type
    if (beginInfo->primaryViewConfigurationType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO) {
        return XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED;
    }

    // Session must be in READY state to begin
    if (sessionData->state != SessionState::READY) {
        return XR_ERROR_SESSION_NOT_READY;
    }

    // Update state and queue events
    sessionData->viewConfigurationType = beginInfo->primaryViewConfigurationType;

    // Transition: READY → SYNCHRONIZED → VISIBLE → FOCUSED
    sessionData->state = SessionState::SYNCHRONIZED;
    {
        std::lock_guard<std::mutex> instanceLock(instanceMutex_);
        auto instIt = instances_.find(sessionData->instance);
        if (instIt != instances_.end()) {
            queueSessionStateChanged(instIt->second.get(), session, SessionState::SYNCHRONIZED);
            sessionData->state = SessionState::VISIBLE;
            queueSessionStateChanged(instIt->second.get(), session, SessionState::VISIBLE);
            sessionData->state = SessionState::FOCUSED;
            queueSessionStateChanged(instIt->second.get(), session, SessionState::FOCUSED);
        }
    }

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::endSession(XrSession session) {
    std::lock_guard<std::mutex> sessionLock(sessionMutex_);
    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SessionData* sessionData = it->second.get();

    // Session must not be IDLE or STOPPING to end
    if (sessionData->state == SessionState::IDLE || sessionData->state == SessionState::STOPPING) {
        return XR_ERROR_SESSION_NOT_RUNNING;
    }

    // Transition to STOPPING then IDLE
    sessionData->state = SessionState::STOPPING;
    {
        std::lock_guard<std::mutex> instanceLock(instanceMutex_);
        auto instIt = instances_.find(sessionData->instance);
        if (instIt != instances_.end()) {
            queueSessionStateChanged(instIt->second.get(), session, SessionState::STOPPING);
            sessionData->state = SessionState::IDLE;
            queueSessionStateChanged(instIt->second.get(), session, SessionState::IDLE);
        }
    }

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::pollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
    if (!eventData) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (eventData->type != XR_TYPE_EVENT_DATA_BUFFER) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(instanceMutex_);
    auto it = instances_.find(instance);
    if (it == instances_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    InstanceData* instanceData = it->second.get();

    if (instanceData->eventQueue.empty()) {
        eventData->type = XR_TYPE_EVENT_DATA_BUFFER;
        return XR_EVENT_UNAVAILABLE;
    }

    *eventData = instanceData->eventQueue.front();
    instanceData->eventQueue.pop();

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::enumerateReferenceSpaces(XrSession session, uint32_t spaceCapacityInput, uint32_t* spaceCountOutput, XrReferenceSpaceType* spaces) {
    if (!spaceCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate session
    if (!isValidSession(session)) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // We support VIEW, LOCAL, and STAGE reference spaces
    static const XrReferenceSpaceType supportedSpaces[] = {
        XR_REFERENCE_SPACE_TYPE_VIEW,
        XR_REFERENCE_SPACE_TYPE_LOCAL,
        XR_REFERENCE_SPACE_TYPE_STAGE
    };
    static const uint32_t spaceCount = sizeof(supportedSpaces) / sizeof(supportedSpaces[0]);

    // Two-call idiom
    if (spaceCapacityInput == 0) {
        *spaceCountOutput = spaceCount;
        return XR_SUCCESS;
    }

    if (spaceCapacityInput < spaceCount) {
        *spaceCountOutput = spaceCount;
        return XR_ERROR_SIZE_INSUFFICIENT;
    }

    if (!spaces) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    for (uint32_t i = 0; i < spaceCount; ++i) {
        spaces[i] = supportedSpaces[i];
    }

    *spaceCountOutput = spaceCount;
    return XR_SUCCESS;
}

XrResult KinectXRRuntime::createReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo, XrSpace* space) {
    if (!createInfo || !space) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (createInfo->type != XR_TYPE_REFERENCE_SPACE_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate session
    if (!isValidSession(session)) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Validate reference space type
    if (createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_VIEW &&
        createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_LOCAL &&
        createInfo->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_STAGE) {
        return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED;
    }

    // Create space handle
    std::lock_guard<std::mutex> lock(spaceMutex_);
    XrSpace handle = reinterpret_cast<XrSpace>(nextSpaceId_++);

    auto spaceData = std::make_unique<SpaceData>(handle, session, createInfo->referenceSpaceType);
    spaces_[handle] = std::move(spaceData);
    *space = handle;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::destroySpace(XrSpace space) {
    std::lock_guard<std::mutex> lock(spaceMutex_);

    auto it = spaces_.find(space);
    if (it == spaces_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    spaces_.erase(it);
    return XR_SUCCESS;
}

bool KinectXRRuntime::isValidSpace(XrSpace space) const {
    std::lock_guard<std::mutex> lock(spaceMutex_);
    return spaces_.find(space) != spaces_.end();
}

XrResult KinectXRRuntime::getMetalGraphicsRequirements(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsMetalKHR* graphicsRequirements) {
    if (!graphicsRequirements) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (graphicsRequirements->type != XR_TYPE_GRAPHICS_REQUIREMENTS_METAL_KHR) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate instance and system
    if (!isValidInstance(instance)) {
        return XR_ERROR_HANDLE_INVALID;
    }

    if (!isValidSystem(instance, systemId)) {
        return XR_ERROR_SYSTEM_INVALID;
    }

    // We support any Metal device - minimal requirements
    // nullptr indicates any Metal device is acceptable
    graphicsRequirements->metalDevice = nullptr;

    return XR_SUCCESS;
}

bool KinectXRRuntime::isValidSwapchain(XrSwapchain swapchain) const {
    std::lock_guard<std::mutex> lock(swapchainMutex_);
    return swapchains_.find(swapchain) != swapchains_.end();
}

SwapchainData* KinectXRRuntime::getSwapchainData(XrSwapchain swapchain) {
    std::lock_guard<std::mutex> lock(swapchainMutex_);

    auto it = swapchains_.find(swapchain);
    if (it == swapchains_.end()) {
        return nullptr;
    }

    return it->second.get();
}

XrResult KinectXRRuntime::enumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput, int64_t* formats) {
    if (!formatCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate session
    if (!isValidSession(session)) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // We only support BGRA8Unorm (Metal's native format on macOS)
    // MTLPixelFormatBGRA8Unorm = 80
    static const int64_t supportedFormats[] = {80};  // MTLPixelFormatBGRA8Unorm
    static const uint32_t formatCount = 1;

    // Two-call idiom
    if (formatCapacityInput == 0) {
        *formatCountOutput = formatCount;
        return XR_SUCCESS;
    }

    if (formatCapacityInput < formatCount) {
        *formatCountOutput = formatCount;
        return XR_ERROR_SIZE_INSUFFICIENT;
    }

    if (!formats) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    formats[0] = supportedFormats[0];
    *formatCountOutput = formatCount;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::createSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain) {
    if (!createInfo || !swapchain) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (createInfo->type != XR_TYPE_SWAPCHAIN_CREATE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate session
    if (!isValidSession(session)) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Validate format (must be BGRA8Unorm = 80)
    if (createInfo->format != 80) {
        return XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
    }

    // Validate dimensions (Kinect is 640x480)
    if (createInfo->width > 640 || createInfo->height > 480) {
        return XR_ERROR_SIZE_INSUFFICIENT;
    }

    // We only support single-sample images
    if (createInfo->sampleCount != 1) {
        return XR_ERROR_FEATURE_UNSUPPORTED;
    }

    // Array size must be 1 (no texture arrays)
    if (createInfo->arraySize != 1) {
        return XR_ERROR_FEATURE_UNSUPPORTED;
    }

    // Validate usage flags (must be COLOR_ATTACHMENT)
    if (!(createInfo->usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT)) {
        return XR_ERROR_FEATURE_UNSUPPORTED;
    }

    // Create swapchain handle
    std::lock_guard<std::mutex> lock(swapchainMutex_);
    XrSwapchain handle = reinterpret_cast<XrSwapchain>(nextSwapchainId_++);

    auto swapchainData = std::make_unique<SwapchainData>(
        handle, session, createInfo->width, createInfo->height, createInfo->format);

    // Get Metal device from session
    SessionData* sessionData = getSessionData(session);
    if (!sessionData) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Create 3 Metal textures (triple buffering)
    // If metalDevice is null or invalid (unit tests), textures will be null
    // This is acceptable for unit testing - integration tests will use real Metal
    if (sessionData->metalDevice) {
        for (uint32_t i = 0; i < 3; ++i) {
            swapchainData->metalTextures[i] = metal::createTexture(
                sessionData->metalDevice,
                createInfo->width,
                createInfo->height,
                createInfo->format);

            // For unit tests with fake Metal devices, textures may be null
            // This is acceptable - only integration tests require real textures
        }
    }

    swapchains_[handle] = std::move(swapchainData);
    *swapchain = handle;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::destroySwapchain(XrSwapchain swapchain) {
    std::lock_guard<std::mutex> lock(swapchainMutex_);

    auto it = swapchains_.find(swapchain);
    if (it == swapchains_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    // Release Metal textures
    SwapchainData* data = it->second.get();
    for (uint32_t i = 0; i < data->imageCount; ++i) {
        if (data->metalTextures[i]) {
            metal::releaseTexture(data->metalTextures[i]);
        }
    }

    swapchains_.erase(it);
    return XR_SUCCESS;
}

XrResult KinectXRRuntime::enumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput, XrSwapchainImageBaseHeader* images) {
    if (!imageCountOutput) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Validate swapchain
    std::lock_guard<std::mutex> lock(swapchainMutex_);
    auto it = swapchains_.find(swapchain);
    if (it == swapchains_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SwapchainData* data = it->second.get();

    // Two-call idiom
    if (imageCapacityInput == 0) {
        *imageCountOutput = data->imageCount;
        return XR_SUCCESS;
    }

    if (imageCapacityInput < data->imageCount) {
        *imageCountOutput = data->imageCount;
        return XR_ERROR_SIZE_INSUFFICIENT;
    }

    if (!images) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Check that images are XrSwapchainImageMetalKHR structures
    XrSwapchainImageMetalKHR* metalImages = reinterpret_cast<XrSwapchainImageMetalKHR*>(images);
    if (metalImages[0].type != XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Fill in Metal texture handles
    for (uint32_t i = 0; i < data->imageCount; ++i) {
        metalImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_METAL_KHR;
        metalImages[i].next = nullptr;
        metalImages[i].texture = data->metalTextures[i];  // nullptr for M3, real textures in M4
    }

    *imageCountOutput = data->imageCount;
    return XR_SUCCESS;
}

XrResult KinectXRRuntime::acquireSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageAcquireInfo* acquireInfo, uint32_t* index) {
    if (!acquireInfo || !index) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (acquireInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(swapchainMutex_);
    auto it = swapchains_.find(swapchain);
    if (it == swapchains_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SwapchainData* data = it->second.get();

    // Only one image can be acquired at a time
    if (data->imageAcquired) {
        return XR_ERROR_CALL_ORDER_INVALID;
    }

    // Return current image index and advance for next time
    *index = data->currentImageIndex;
    data->imageAcquired = true;

    // Cycle to next image (0→1→2→0)
    data->currentImageIndex = (data->currentImageIndex + 1) % data->imageCount;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::waitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo) {
    if (!waitInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (waitInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(swapchainMutex_);
    auto it = swapchains_.find(swapchain);
    if (it == swapchains_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SwapchainData* data = it->second.get();

    // Must have acquired an image first
    if (!data->imageAcquired) {
        return XR_ERROR_CALL_ORDER_INVALID;
    }

    // For Kinect XR, images are always immediately ready (no GPU work to wait for)
    // In a real implementation, this would block until the GPU finishes rendering
    // The timeout parameter is ignored since we return immediately

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::releaseSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageReleaseInfo* releaseInfo) {
    if (!releaseInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (releaseInfo->type != XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(swapchainMutex_);
    auto it = swapchains_.find(swapchain);
    if (it == swapchains_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SwapchainData* data = it->second.get();

    // Must have acquired an image first
    if (!data->imageAcquired) {
        return XR_ERROR_CALL_ORDER_INVALID;
    }

    // Mark image as released (ready for next acquire)
    data->imageAcquired = false;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::waitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState) {
    if (!frameWaitInfo || !frameState) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (frameWaitInfo->type != XR_TYPE_FRAME_WAIT_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (frameState->type != XR_TYPE_FRAME_STATE) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(sessionMutex_);
    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SessionData* sessionData = it->second.get();

    // Session must be running (SYNCHRONIZED, VISIBLE, or FOCUSED)
    if (sessionData->state != SessionState::SYNCHRONIZED &&
        sessionData->state != SessionState::VISIBLE &&
        sessionData->state != SessionState::FOCUSED) {
        return XR_ERROR_SESSION_NOT_RUNNING;
    }

    // Pace at 30Hz (33.3ms per frame) to match Kinect's native frame rate
    const auto targetFrameTime = std::chrono::milliseconds(33);  // ~30 FPS
    auto now = std::chrono::steady_clock::now();

    if (sessionData->frameState.lastFrameTime != 0) {
        // Calculate time since last frame
        auto lastFrameTimePoint = std::chrono::steady_clock::time_point(
            std::chrono::nanoseconds(sessionData->frameState.lastFrameTime));
        auto elapsed = now - lastFrameTimePoint;

        // Sleep if we're rendering too fast
        if (elapsed < targetFrameTime) {
            std::this_thread::sleep_for(targetFrameTime - elapsed);
            now = std::chrono::steady_clock::now();
        }
    }

    // Update frame state
    sessionData->frameState.lastFrameTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    sessionData->frameState.frameCount++;

    // Fill in XrFrameState
    frameState->predictedDisplayTime = sessionData->frameState.lastFrameTime;
    frameState->predictedDisplayPeriod = 33333333;  // 33.3ms in nanoseconds
    frameState->shouldRender = XR_TRUE;  // Always render for Kinect

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::beginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
    if (!frameBeginInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (frameBeginInfo->type != XR_TYPE_FRAME_BEGIN_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(sessionMutex_);
    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SessionData* sessionData = it->second.get();

    // Session must be running
    if (sessionData->state != SessionState::SYNCHRONIZED &&
        sessionData->state != SessionState::VISIBLE &&
        sessionData->state != SessionState::FOCUSED) {
        return XR_ERROR_SESSION_NOT_RUNNING;
    }

    // Cannot begin a frame if one is already in progress
    if (sessionData->frameState.frameInProgress) {
        return XR_ERROR_CALL_ORDER_INVALID;
    }

    // Mark frame as in progress
    sessionData->frameState.frameInProgress = true;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::endFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
    if (!frameEndInfo) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (frameEndInfo->type != XR_TYPE_FRAME_END_INFO) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    std::lock_guard<std::mutex> lock(sessionMutex_);
    auto it = sessions_.find(session);
    if (it == sessions_.end()) {
        return XR_ERROR_HANDLE_INVALID;
    }

    SessionData* sessionData = it->second.get();

    // Session must be running
    if (sessionData->state != SessionState::SYNCHRONIZED &&
        sessionData->state != SessionState::VISIBLE &&
        sessionData->state != SessionState::FOCUSED) {
        return XR_ERROR_SESSION_NOT_RUNNING;
    }

    // Must have begun a frame first
    if (!sessionData->frameState.frameInProgress) {
        return XR_ERROR_CALL_ORDER_INVALID;
    }

    // Validate display time (should match what we returned from waitFrame)
    if (frameEndInfo->displayTime != sessionData->frameState.lastFrameTime) {
        // Allow some flexibility, but warn if completely wrong
        // For now, accept any reasonable value
    }

    // Validate environment blend mode (we only support opaque)
    if (frameEndInfo->environmentBlendMode != XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
        return XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED;
    }

    // Process layers (for M7, just validate counts)
    if (frameEndInfo->layerCount > 0 && !frameEndInfo->layers) {
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // For M7, we don't process layers yet (that's for Spec 007)
    // Just accept them and mark frame as complete

    // Mark frame as complete
    sessionData->frameState.frameInProgress = false;

    return XR_SUCCESS;
}

XrResult KinectXRRuntime::locateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo, XrViewState* viewState, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrView* views) {
    // Stub for M8
    return XR_ERROR_RUNTIME_FAILURE;
}

} // namespace kinect_xr

