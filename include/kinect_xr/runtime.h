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
};

} // namespace kinect_xr
