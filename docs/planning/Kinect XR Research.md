# Comprehensive Research: OpenXR Framework and Depth Sensing Capabilities

## Introduction to OpenXR

OpenXR is an open-source, royalty-free standard designed to provide unified access to virtual reality (VR) and augmented reality (AR) platforms and devices. Developed by a working group managed by the Khronos Group consortium, OpenXR addresses the fragmentation challenge in the XR industry by creating a common API that allows applications to work across various hardware platforms[^1_11]. First announced in February 2017 during GDC, the standard reached version 1.0 in July 2019, with the most recent significant release being OpenXR 1.1.47 on April 8, 2025[^1_7][^1_11].

The framework enables developers to write code once that can then be deployed across multiple hardware platforms, including HoloLens 2, Windows Mixed Reality headsets, Android XR devices, and other VR/AR systems. This cross-platform compatibility significantly reduces development effort and increases the potential reach of XR applications.

## Official Specifications and API Structure

### Core Architecture

The fundamental architecture of OpenXR consists of several key elements[^1_11]:

- **XrSpace**: A representation of 3D space
- **XrInstance**: A representation of the OpenXR runtime
- **XrSystemId**: A representation of devices, including VR/AR hardware and controllers
- **XrActions**: Used to handle user inputs
- **XrSession**: Represents the interaction session between the application and user

The official specifications and documentation are maintained in the OpenXR-Registry repository, which serves as the backing store for the web view of the registry and contains generated specifications, reference pages, and reference cards[^1_12].

### API Registry and Extensions

OpenXR defines an API Registry that formally specifies function prototypes, structures, enumerants, and other API aspects[^1_2]. The registry is crucial for both implementing OpenXR runtimes and developing applications that use the API.

Extensions play a vital role in the OpenXR ecosystem, allowing platform-specific features while maintaining cross-platform compatibility. The OpenXR specification defines different extension types[^1_1]:

- **Vendor extensions (XR_MSFT_...)**: Enable per-vendor innovation in hardware or software features
- **Experimental vendor extensions (XR_MSFTX_...)**: Preview extensions for developer feedback
- **Cross-vendor EXT extensions**: Defined and implemented by multiple companies
- **Official KHR extensions**: Officially ratified Khronos extensions


### Latest Specifications

The OpenXR 1.1.47 release (April 2025) introduced several new vendor extensions and modifications to the XML schema for interaction profiles[^1_7]. This includes extensions for:

- Detached controllers
- Spatial sensing
- Spatial anchors
- Spatial anchor sharing
- Spatial scene
- Spatial mesh


## Standardized Spatial Interfaces and Depth Sensing

### Depth Sensing APIs

OpenXR provides several extensions for working with depth data, which is crucial for spatial mapping and object placement in mixed reality applications:

#### XR_KHR_composition_layer_depth

This extension is fundamental for submitting depth buffers along with color information. Microsoft's best practices explicitly recommend: "Always use XR_KHR_composition_layer_depth extension and submit the depth buffer together with the projection layer when submitting a frame to xrEndFrame"[^1_14]. This enables hardware depth reprojection on devices like HoloLens 2, significantly improving hologram stability.

#### XR_META_environment_depth

Meta's implementation allows access to depth sensor data from devices like the Quest 3. The extension defines data structures such as XrEnvironmentDepthSwapchainCreateInfoMETA and functions like xrAcquireEnvironmentDepthImageMETA to retrieve depth information[^1_4].

One developer described their experience: "xrAcquireEnvironmentDepthImageMETA returns 0 (successful) every frame, and I can see that data from XrEnvironmentDepthImageMETA is correct (views show fov and pose correctly, and swapchainindex updates every frame)"[^1_4].

### Spatial UI Interfaces

Android XR provides a standardized approach to spatial UI through components like[^1_3]:

- **Spatial panels**: Fundamental building blocks for XR apps that serve as containers for UI elements and interactive components
- **Dynamic scaling**: Automatically adjusts UI element size based on distance from the user
- **Movement constraints**: Default panel movement limits (minimum depth: 0.75 meters, maximum depth: 5 meters)


### Depth Data Formats and Processing

For developers working with depth data, there are several considerations around formats and processing:

1. **Depth formats**: OpenXR typically provides depth data as textures in the same format as described in the XR_KHR_composition_layer_depth extension[^1_4].
2. **Depth range**: Microsoft recommends using a narrower depth range (e.g., 0.1 to 20 meters) for better hologram stability, and using reversed-Z for more uniform depth resolution[^1_14].
3. **Depth buffer format**: On HoloLens 2, using DXGI_FORMAT_D16_UNORM depth format helps achieve better frame rate and performance, though with less depth resolution than 24-bit buffers[^1_14].

## Creating Compliant OpenXR Runtimes

### Development Setup

To get started with OpenXR in an existing project, you can include the OpenXR loader, which discovers the active OpenXR runtime on the device and provides access to core functions and extension functions[^1_5].

Two primary approaches are:

1. **Reference the official OpenXR NuGet package** in your Visual Studio project, which provides access to OpenXR 1.0 core features and published KHR, EXT, and MSFT extensions.
2. **Include the official OpenXR loader source** from the Khronos GitHub repo if you want to build the loader yourself.

### Graphics API Integration

OpenXR is designed to work with various graphics APIs, including Vulkan, Direct3D 11, Direct3D 12, OpenGL, and OpenGL ES[^1_13]. When integrating with these APIs, developers need to create swapchains for rendering, similar to rendering to a 2D display[^1_10].

The OpenXR tutorial provides detailed guidance on creating and using swapchains, including code examples for setting up shaders and graphics pipelines[^1_10].

### Best Practices for Runtime Development

Microsoft provides several best practices for visual quality and stability in OpenXR applications[^1_14]:

1. **Gamma-correct rendering**: Ensure your rendering pipeline matches swapchain format with render-target view format
2. **Depth buffer submission**: Always use XR_KHR_composition_layer_depth when available
3. **Appropriate depth range**: Choose a reasonable depth range to improve stability
4. **Environment blend modes**: Prepare for different modes using xrEnumerateEnvironmentBlendModes
5. **Reference space**: Use XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT when supported
6. **Single projection layer**: Optimize for a single projection layer to improve performance
7. **Texture array rendering**: Render using texture arrays and VPRT (Viewport Array/Instancing) for stereoscopic rendering

## Debugging Tools for OpenXR Development

### OpenXR Explorer

OpenXR Explorer is a valuable debug tool for OpenXR developers that provides several key features[^1_6]:

1. **Runtime switching**: A simple dropdown to manage switching between runtimes, with a configurable list for non-standard installations
2. **Command-line switching**: The xrsetruntime utility allows switching from the command line
3. **Runtime information**: Displays common lists and enumerations, with quick links to relevant sections of the OpenXR specification
4. **Command-line interface**: Most GUI functionality is also available in text format when used from the command line

The tool is available for both Windows and Linux systems and can be downloaded from the releases tab of the GitHub repository[^1_6].

## Example Implementations for Depth Sensing

### MagicLeap Depth Camera Example

MagicLeap provides a comprehensive example of accessing and visualizing depth data using OpenXR[^1_9]. The implementation includes:

1. **DepthCameraExample** class: Configures and accesses the depth sensor using the MagicLeap.OpenXR.Features.PixelSensors namespace
2. **DepthStreamVisualizer** class: Renders depth data using custom shaders
3. **Processing options**: Support for raw depth, depth with confidence, and depth with flags modes

The example demonstrates how to:

- Request appropriate permissions
- Initialize the depth sensor
- Configure update rates for different depth ranges
- Process and visualize depth data

```csharp
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using MagicLeap.Android;
using Unity.Collections;
using UnityEngine;
using UnityEngine.XR.MagicLeap;
using UnityEngine.XR.OpenXR;
using MagicLeap.OpenXR.Features.PixelSensors;

public class DepthCameraExample : MonoBehaviour
{
    [Header("General Configuration")]
    [Tooltip("If True will return a raw depth image. If False will return depth32")]
    public bool UseRawDepth;
    [Range(0.2f, 5.00f)] public float DepthRange;
    [Header("ShortRange =&lt; 1m")] public ShortRangeUpdateRate SRUpdateRate;
    [Header("LongRange &gt; 1m")] public LongRangeUpdateRate LRUpdateRate;
    
    // Implementation details...
}
```


### Meta Quest 3 Depth API Implementation

The search results also reference an implementation for accessing depth data from the Meta Quest 3 using the XR_META_environment_depth extension[^1_4]. The process involves:

1. Creating a depth swapchain with XrEnvironmentDepthSwapchainCreateInfoMETA
2. Acquiring depth images with xrAcquireEnvironmentDepthImageMETA
3. Accessing the depth textures through xrEnumerateEnvironmentDepthSwapchainImagesMETA

## Conclusion

OpenXR has established itself as a critical standard for cross-platform XR development, offering a unified API that works across a wide range of hardware. Its extensible architecture allows for vendor-specific innovations while maintaining a consistent core API.

For depth sensing applications, OpenXR provides robust support through extensions like XR_KHR_composition_layer_depth and XR_META_environment_depth, enabling developers to create spatially aware applications that can understand and interact with the user's environment.

As the XR industry continues to evolve, OpenXR's importance will likely grow, with new extensions and capabilities added to support emerging hardware features. Developers who invest in understanding the OpenXR standard and best practices will be well-positioned to create high-quality, cross-platform XR experiences.

[^1_1]: https://learn.microsoft.com/en-us/windows/mixed-reality/develop/native/openxr

[^1_2]: https://dochavez.github.io/Documenting-with-Docusaurus-V2.-/docs/doc2/

[^1_3]: https://developer.android.com/design/ui/xr/guides/spatial-ui

[^1_4]: https://community.khronos.org/t/quest-3-depth-api-with-xr-meta-environment-depth/110808

[^1_5]: https://learn.microsoft.com/en-us/windows/mixed-reality/develop/native/openxr-getting-started

[^1_6]: https://github.com/maluoi/openxr-explorer

[^1_7]: https://github.com/KhronosGroup/OpenXR-Registry/releases

[^1_8]: https://communityforums.atmeta.com/t5/OpenXR-Development/DEVICE-LOST-when-trying-to-use-XR-KHR-composition-layer-depth/td-p/1227351

[^1_9]: https://developer-docs.magicleap.cloud/docs/guides/unity-openxr/pixel-sensor/depth-camera-example/

[^1_10]: https://openxr-tutorial.com/android/vulkan/3-graphics.html

[^1_11]: https://en.wikipedia.org/wiki/OpenXR

[^1_12]: https://github.com/KhronosGroup/OpenXR-Registry

[^1_13]: https://openxr-tutorial.com/index.html

[^1_14]: https://learn.microsoft.com/en-us/windows/mixed-reality/develop/native/openxr-best-practices

[^1_15]: https://docs.unity3d.com/Packages/com.unity.xr.openxr@1.3/api/UnityEngine.XR.OpenXR.OpenXRSettings.html

[^1_16]: https://github.com/KhronosGroup/OpenXR-Docs/blob/master/specification/README.md

[^1_17]: https://mbucchia.github.io/OpenXR-Toolkit/

[^1_18]: https://amini-allight.org/post/openxr-tutorial-part-1

[^1_19]: https://developer.vive.com/resources/openxr/unity/common-interface/

[^1_20]: https://github.com/KhronosGroup/OpenXR-Docs

[^1_21]: https://docs.unity3d.com/Packages/com.unity.xr.meta-openxr@2.1/manual/architecture.html

[^1_22]: https://developer.android.com/develop/xr/openxr/extensions/XR_ANDROID_depth_texture

[^1_23]: https://academy.vrex.no/knowledge-base/openxr/

[^1_24]: https://community.khronos.org/t/debugging-performance-issues/110894

[^1_25]: https://github.com/maluoi/OpenXRSamples

[^1_26]: https://www.qualcomm.com/news/onq/2020/11/openxr-and-introduction-khronos-group

[^1_27]: https://openxr-tutorial.com/linux/opengl/1-introduction.html

[^1_28]: https://www.reddit.com/r/OpenXR/comments/1c4peju/the_khronos_group_releases_openxr_11_specification/

[^1_29]: https://docs.unity3d.com/Packages/com.unity.xr.openxr@1.2/api/UnityEngine.XR.OpenXR.OpenXRSettings.html

[^1_30]: https://developers.meta.com/horizon/documentation/native/android/mobile-depth/

[^1_31]: https://docs.unity3d.com/Packages/com.unity.xr.openxr@1.2/api/UnityEngine.XR.OpenXR.Features.RuntimeDebugger.RuntimeDebuggerOpenXRFeature.html

[^1_32]: https://venturebeat.com/business/khronos-group-releases-early-openxr-spec-for-ar-and-vr-hardware-standards/

[^1_33]: https://dev.epicgames.com/documentation/en-us/unreal-engine/openxr-prerequisites?application_version=4.27

[^1_34]: https://docs.unity3d.com/Packages/com.unity.xr.openxr@1.6/api/UnityEngine.XR.OpenXR.OpenXRSettings.html

[^1_35]: https://github.com/oculus-samples/Unity-DepthAPI

[^1_36]: https://www.intel.com/content/dam/develop/external/us/en/documents/gdc-2019-khronos-openxr-presentation-807276.pdf

[^1_37]: https://developer-docs.magicleap.cloud/docs/guides/remote-rendering/remote-rendering-openxr/

[^1_38]: https://openxr-tutorial.com/linux/opengl/5-extensions.html

[^1_39]: https://developer.android.com/develop/xr/openxr/extensions

[^1_40]: https://github.com/KhronosGroup/OpenXR-Docs/blob/main/CHANGELOG.Docs.md

[^1_41]: https://www.reddit.com/r/PavlovGame/comments/147r9k6/if_you_are_on_pc_set_your_runtime_to_openxr/

[^1_42]: https://steamcommunity.com/app/2709120/discussions/0/599640352765283701/

[^1_43]: https://forum.dcs.world/topic/296983-openxr-toolkit-tuning-guide-updated-210223/

[^1_44]: https://learn.microsoft.com/en-us/windows/mixed-reality/develop/native/openxr-getting-started

[^1_45]: https://openxr-tutorial.com/index.html

[^1_46]: https://www.reddit.com/r/dcsworld/comments/12hfh1d/openxr_toolkit_setup_guide_with_quest_2/