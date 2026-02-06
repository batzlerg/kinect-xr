#pragma once

#include <openxr/openxr.h>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace kinect_xr {

// Forward declaration for internal instance data
struct InstanceData {
    XrInstance handle;
    std::string applicationName;
    uint32_t applicationVersion;
    std::string engineName;
    uint32_t engineVersion;
    uint32_t apiVersion;

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
};

} // namespace kinect_xr
