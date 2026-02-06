#pragma once

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <queue>

namespace kinect_xr {

// Forward declarations for internal data structures
struct InstanceData;
struct SystemData;
struct SessionData;
struct SpaceData;
struct SwapchainData;
struct FrameState;

// Space data
struct SpaceData {
    XrSpace handle;
    XrSession session;  // Parent session
    XrReferenceSpaceType referenceSpaceType;

    SpaceData(XrSpace h, XrSession sess, XrReferenceSpaceType type)
        : handle(h)
        , session(sess)
        , referenceSpaceType(type) {}
};

// System data for Kinect XR
struct SystemData {
    XrSystemId systemId;
    XrFormFactor formFactor;

    SystemData(XrSystemId id)
        : systemId(id)
        , formFactor(XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) {}
};

// Session state
enum class SessionState {
    IDLE,
    READY,
    SYNCHRONIZED,
    VISIBLE,
    FOCUSED,
    STOPPING
};

// Swapchain data
struct SwapchainData {
    XrSwapchain handle;
    XrSession session;  // Parent session
    uint32_t width;
    uint32_t height;
    int64_t format;  // Metal pixel format (BGRA8Unorm)
    uint32_t imageCount;  // Always 3 (triple buffering)
    uint32_t currentImageIndex;  // Current acquired image
    bool imageAcquired;  // Is an image currently acquired?

    // Metal textures (stored as void* to avoid Metal headers)
    void* metalTextures[3];  // MTLTexture pointers

    SwapchainData(XrSwapchain h, XrSession sess, uint32_t w, uint32_t ht, int64_t fmt)
        : handle(h)
        , session(sess)
        , width(w)
        , height(ht)
        , format(fmt)
        , imageCount(3)
        , currentImageIndex(0)
        , imageAcquired(false)
        , metalTextures{nullptr, nullptr, nullptr} {}
};

// Frame state for frame loop timing
struct FrameState {
    bool frameInProgress;  // Between xrBeginFrame and xrEndFrame
    XrTime lastFrameTime;  // Last predicted display time
    uint64_t frameCount;  // Total frames rendered

    FrameState()
        : frameInProgress(false)
        , lastFrameTime(0)
        , frameCount(0) {}
};

// Frame cache for Kinect RGB + depth data
// Thread-safe storage for latest frames from Kinect callbacks
struct FrameCache {
    std::mutex mutex;  // Protects all fields below

    // RGB frame (640x480, RGB888 format)
    std::vector<uint8_t> rgbData;  // 640 * 480 * 3 = 921600 bytes
    uint32_t rgbTimestamp;
    bool rgbValid;

    // Depth frame (640x480, 11-bit values in uint16_t)
    std::vector<uint16_t> depthData;  // 640 * 480 = 307200 uint16_t
    uint32_t depthTimestamp;
    bool depthValid;

    FrameCache()
        : rgbData(640 * 480 * 3)
        , rgbTimestamp(0)
        , rgbValid(false)
        , depthData(640 * 480)
        , depthTimestamp(0)
        , depthValid(false) {}
};

// Session data
struct SessionData {
    XrSession handle;
    XrInstance instance;  // Parent instance
    XrSystemId systemId;  // Associated system
    SessionState state;
    XrViewConfigurationType viewConfigurationType;

    // Metal graphics binding (stored as void* to avoid Metal headers in this header)
    void* metalCommandQueue;
    void* metalDevice;  // MTLDevice pointer (needed for swapchain creation)

    // Frame state
    FrameState frameState;

    // Kinect frame cache (latest RGB + depth from callbacks)
    FrameCache frameCache;

    SessionData(XrSession h, XrInstance inst, XrSystemId sysId)
        : handle(h)
        , instance(inst)
        , systemId(sysId)
        , state(SessionState::IDLE)
        , viewConfigurationType(XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM)
        , metalCommandQueue(nullptr)
        , metalDevice(nullptr) {}
};

// Instance data
struct InstanceData {
    XrInstance handle;
    std::string applicationName;
    uint32_t applicationVersion;
    std::string engineName;
    uint32_t engineVersion;
    uint32_t apiVersion;

    // System is per-instance (Kinect doesn't change while runtime is active)
    std::unique_ptr<SystemData> system;

    // Event queue for this instance
    std::queue<XrEventDataBuffer> eventQueue;

    InstanceData(XrInstance h) : handle(h), applicationVersion(0), engineVersion(0), apiVersion(XR_CURRENT_API_VERSION) {}
};

/**
 * KinectXRRuntime - Main runtime singleton managing OpenXR instances
 *
 * This class manages the lifecycle of OpenXR instances and provides
 * the core runtime functionality for the Kinect XR runtime.
 */
class KinectXRRuntime {
public:
    static KinectXRRuntime& getInstance();

    // Instance management
    XrResult createInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance);
    XrResult destroyInstance(XrInstance instance);

    // Instance validation
    bool isValidInstance(XrInstance instance) const;
    InstanceData* getInstanceData(XrInstance instance);

    // System management
    XrResult getSystem(XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId);
    XrResult getSystemProperties(XrInstance instance, XrSystemId systemId, XrSystemProperties* properties);

    // System validation
    bool isValidSystem(XrInstance instance, XrSystemId systemId) const;

    // Session management
    XrResult createSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session);
    XrResult destroySession(XrSession session);

    // Session validation
    bool isValidSession(XrSession session) const;
    SessionData* getSessionData(XrSession session);

    // Session lifecycle
    XrResult beginSession(XrSession session, const XrSessionBeginInfo* beginInfo);
    XrResult endSession(XrSession session);

    // Event queue
    XrResult pollEvent(XrInstance instance, XrEventDataBuffer* eventData);

    // Reference space management
    XrResult enumerateReferenceSpaces(XrSession session, uint32_t spaceCapacityInput, uint32_t* spaceCountOutput, XrReferenceSpaceType* spaces);
    XrResult createReferenceSpace(XrSession session, const XrReferenceSpaceCreateInfo* createInfo, XrSpace* space);
    XrResult destroySpace(XrSpace space);

    // Space validation
    bool isValidSpace(XrSpace space) const;

    // Graphics requirements
    XrResult getMetalGraphicsRequirements(XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsMetalKHR* graphicsRequirements);

    // Swapchain management
    XrResult enumerateSwapchainFormats(XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput, int64_t* formats);
    XrResult createSwapchain(XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain);
    XrResult destroySwapchain(XrSwapchain swapchain);
    XrResult enumerateSwapchainImages(XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput, XrSwapchainImageBaseHeader* images);
    XrResult acquireSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageAcquireInfo* acquireInfo, uint32_t* index);
    XrResult waitSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo);
    XrResult releaseSwapchainImage(XrSwapchain swapchain, const XrSwapchainImageReleaseInfo* releaseInfo);

    // Swapchain validation
    bool isValidSwapchain(XrSwapchain swapchain) const;
    SwapchainData* getSwapchainData(XrSwapchain swapchain);

    // Frame loop timing
    XrResult waitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState);
    XrResult beginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo);
    XrResult endFrame(XrSession session, const XrFrameEndInfo* frameEndInfo);

    // View poses
    XrResult locateViews(XrSession session, const XrViewLocateInfo* viewLocateInfo, XrViewState* viewState, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrView* views);

    // Delete copy and move constructors/operators
    KinectXRRuntime(const KinectXRRuntime&) = delete;
    KinectXRRuntime& operator=(const KinectXRRuntime&) = delete;
    KinectXRRuntime(KinectXRRuntime&&) = delete;
    KinectXRRuntime& operator=(KinectXRRuntime&&) = delete;

private:
    KinectXRRuntime() = default;
    ~KinectXRRuntime() = default;

    mutable std::mutex instanceMutex_;
    std::unordered_map<XrInstance, std::unique_ptr<InstanceData>> instances_;
    uint64_t nextInstanceId_ = 1;
    uint64_t nextSystemId_ = 1;

    mutable std::mutex sessionMutex_;
    std::unordered_map<XrSession, std::unique_ptr<SessionData>> sessions_;
    uint64_t nextSessionId_ = 1;

    mutable std::mutex spaceMutex_;
    std::unordered_map<XrSpace, std::unique_ptr<SpaceData>> spaces_;
    uint64_t nextSpaceId_ = 1;

    mutable std::mutex swapchainMutex_;
    std::unordered_map<XrSwapchain, std::unique_ptr<SwapchainData>> swapchains_;
    uint64_t nextSwapchainId_ = 1;
};

} // namespace kinect_xr
