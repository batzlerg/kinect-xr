#pragma once

#include <openxr/openxr.h>
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

// Session data
struct SessionData {
    XrSession handle;
    XrInstance instance;  // Parent instance
    XrSystemId systemId;  // Associated system
    SessionState state;
    XrViewConfigurationType viewConfigurationType;

    // Metal graphics binding (stored as void* to avoid Metal headers in this header)
    void* metalCommandQueue;

    SessionData(XrSession h, XrInstance inst, XrSystemId sysId)
        : handle(h)
        , instance(inst)
        , systemId(sysId)
        , state(SessionState::IDLE)
        , viewConfigurationType(XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM)
        , metalCommandQueue(nullptr) {}
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
};

} // namespace kinect_xr
