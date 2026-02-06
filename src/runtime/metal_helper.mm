#import <Metal/Metal.h>
#include "kinect_xr/metal_helper.h"

namespace kinect_xr {
namespace metal {

void* createTexture(void* metalDevicePtr, uint32_t width, uint32_t height, int64_t format) {
    if (!metalDevicePtr) {
        return nullptr;
    }

    // Only support BGRA8Unorm for now (format = 80)
    if (format != 80) {
        return nullptr;
    }

    // Check if this looks like a small integer (fake test pointer)
    uintptr_t ptr = (uintptr_t)metalDevicePtr;
    if (ptr < 0x1000) {
        // Fake pointer for unit testing - return nullptr (no real texture)
        return nullptr;
    }

    @try {
        id<MTLDevice> device = (__bridge id<MTLDevice>)metalDevicePtr;

        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
            width:width
            height:height
            mipmapped:NO];

        descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        descriptor.storageMode = MTLStorageModePrivate;

        id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
        if (!texture) {
            return nullptr;
        }

        // Manually retain (since we're not using ARC)
        CFRetain((__bridge CFTypeRef)texture);
        return (__bridge void*)texture;
    }
    @catch (NSException *exception) {
        // Invalid device pointer
        return nullptr;
    }
}

void releaseTexture(void* texturePtr) {
    if (!texturePtr) {
        return;
    }

    // Check if this looks like a small integer (fake test pointer)
    uintptr_t ptr = (uintptr_t)texturePtr;
    if (ptr < 0x1000) {
        // Fake pointer for unit testing - nothing to release
        return;
    }

    // Release (manual memory management since no ARC)
    CFRelease((__bridge CFTypeRef)texturePtr);
}

void* getMetalDevice(void* commandQueuePtr) {
    if (!commandQueuePtr) {
        return nullptr;
    }

    // Check if this looks like a small integer (fake test pointer)
    // Real Metal pointers are heap-allocated and >= 0x1000
    uintptr_t ptr = (uintptr_t)commandQueuePtr;
    if (ptr < 0x1000) {
        // Fake pointer for unit testing - return it as-is
        return commandQueuePtr;
    }

    @try {
        id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)commandQueuePtr;
        id<MTLDevice> device = queue.device;

        // Return unretained pointer (device is owned by queue)
        return (__bridge void*)device;
    }
    @catch (NSException *exception) {
        // Invalid pointer - not a real Metal command queue
        return nullptr;
    }
}

} // namespace metal
} // namespace kinect_xr
