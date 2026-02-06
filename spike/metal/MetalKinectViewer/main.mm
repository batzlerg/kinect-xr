#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <kinect_xr/device.h>
#include <iostream>
#include <atomic>
#include <chrono>

// Metal renderer that displays Kinect RGB and depth textures
@interface Renderer : NSObject <MTKViewDelegate> {
    std::atomic<uint64_t> frameCount_;
    std::chrono::steady_clock::time_point lastFpsTime_;
    double currentFps_;
}
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> rgbPipeline;
@property (nonatomic, strong) id<MTLRenderPipelineState> depthPipeline;
@property (nonatomic, strong) id<MTLTexture> rgbTexture;
@property (nonatomic, strong) id<MTLTexture> depthTexture;
@property (nonatomic, strong) id<MTLSamplerState> sampler;
@property (nonatomic, strong) dispatch_queue_t textureUpdateQueue;
- (void)updateRGBTextureWithData:(const void*)data;
- (void)updateDepthTextureWithData:(const void*)data;
- (double)getCurrentFps;
@end

@implementation Renderer

- (instancetype)initWithMetalKitView:(MTKView *)mtkView {
    self = [super init];
    if (self) {
        _device = mtkView.device;
        _commandQueue = [_device newCommandQueue];
        _textureUpdateQueue = dispatch_queue_create("com.kinect.textureUpdate", DISPATCH_QUEUE_SERIAL);

        // Initialize performance tracking
        frameCount_ = 0;
        currentFps_ = 0.0;
        lastFpsTime_ = std::chrono::steady_clock::now();

        // Create RGB texture (640x480 Kinect resolution)
        MTLTextureDescriptor *rgbTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                   width:640
                                                                                                  height:480
                                                                                               mipmapped:NO];
        rgbTextureDesc.usage = MTLTextureUsageShaderRead;
        _rgbTexture = [_device newTextureWithDescriptor:rgbTextureDesc];
        if (!_rgbTexture) {
            NSLog(@"Failed to create RGB texture");
            return nil;
        }

        // Create depth texture (640x480, R16Uint for 11-bit depth values)
        MTLTextureDescriptor *depthTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR16Uint
                                                                                                     width:640
                                                                                                    height:480
                                                                                                 mipmapped:NO];
        depthTextureDesc.usage = MTLTextureUsageShaderRead;
        _depthTexture = [_device newTextureWithDescriptor:depthTextureDesc];
        if (!_depthTexture) {
            NSLog(@"Failed to create depth texture");
            return nil;
        }

        // Create sampler
        MTLSamplerDescriptor *samplerDesc = [[MTLSamplerDescriptor alloc] init];
        samplerDesc.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
        _sampler = [_device newSamplerStateWithDescriptor:samplerDesc];

        // Load shaders and create pipeline
        NSError *error = nil;
        id<MTLLibrary> library = [_device newDefaultLibrary];
        if (!library) {
            NSLog(@"Failed to create Metal library");
            return nil;
        }

        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
        id<MTLFunction> rgbFragmentFunction = [library newFunctionWithName:@"textureFragment"];
        id<MTLFunction> depthFragmentFunction = [library newFunctionWithName:@"depthFragment"];

        // RGB pipeline
        MTLRenderPipelineDescriptor *rgbPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        rgbPipelineDesc.vertexFunction = vertexFunction;
        rgbPipelineDesc.fragmentFunction = rgbFragmentFunction;
        rgbPipelineDesc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat;

        _rgbPipeline = [_device newRenderPipelineStateWithDescriptor:rgbPipelineDesc error:&error];
        if (!_rgbPipeline) {
            NSLog(@"Failed to create RGB pipeline state: %@", error);
            return nil;
        }

        // Depth pipeline
        MTLRenderPipelineDescriptor *depthPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        depthPipelineDesc.vertexFunction = vertexFunction;
        depthPipelineDesc.fragmentFunction = depthFragmentFunction;
        depthPipelineDesc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat;

        _depthPipeline = [_device newRenderPipelineStateWithDescriptor:depthPipelineDesc error:&error];
        if (!_depthPipeline) {
            NSLog(@"Failed to create depth pipeline state: %@", error);
            return nil;
        }

        std::cout << "Renderer initialized with 640x480 BGRA+R16Uint textures" << std::endl;
    }
    return self;
}

- (void)updateRGBTextureWithData:(const void*)data {
    if (!data || !_rgbTexture) return;

    // Kinect RGB is 640x480 RGB888, need to convert to BGRA8
    dispatch_async(_textureUpdateQueue, ^{
        const uint8_t* rgb = (const uint8_t*)data;
        const size_t width = 640;
        const size_t height = 480;
        const size_t bgraSize = width * height * 4;

        // Allocate BGRA buffer
        uint8_t* bgra = (uint8_t*)malloc(bgraSize);
        if (!bgra) return;

        // Convert RGB to BGRA
        for (size_t i = 0; i < width * height; i++) {
            bgra[i * 4 + 0] = rgb[i * 3 + 2];  // B
            bgra[i * 4 + 1] = rgb[i * 3 + 1];  // G
            bgra[i * 4 + 2] = rgb[i * 3 + 0];  // R
            bgra[i * 4 + 3] = 255;             // A
        }

        // Upload to texture
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [self.rgbTexture replaceRegion:region
                           mipmapLevel:0
                             withBytes:bgra
                           bytesPerRow:width * 4];

        free(bgra);
    });
}

- (void)updateDepthTextureWithData:(const void*)data {
    if (!data || !_depthTexture) return;

    // Kinect depth is 640x480 16-bit values (11-bit depth, 0-2047)
    dispatch_async(_textureUpdateQueue, ^{
        const uint16_t* depth = (const uint16_t*)data;
        const size_t width = 640;
        const size_t height = 480;

        // Upload directly as R16Uint (shader will visualize)
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        [self.depthTexture replaceRegion:region
                             mipmapLevel:0
                               withBytes:depth
                             bytesPerRow:width * sizeof(uint16_t)];
    });
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

- (void)drawInMTKView:(MTKView *)view {
    // Update FPS counter
    frameCount_++;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsTime_).count();
    if (elapsed >= 1000) {
        currentFps_ = (double)frameCount_.load() / (elapsed / 1000.0);
        if (frameCount_ % 60 == 0) {
            std::cout << "Render FPS: " << currentFps_ << std::endl;
        }
        frameCount_ = 0;
        lastFpsTime_ = now;
    }

    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

    // Get render pass descriptor from view
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if (renderPassDescriptor) {
        // Create render encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];

        CGSize drawableSize = view.drawableSize;
        float halfWidth = drawableSize.width / 2.0f;

        // Left half: RGB
        MTLViewport leftViewport = {0, 0, halfWidth, drawableSize.height, 0, 1};
        [renderEncoder setViewport:leftViewport];
        [renderEncoder setRenderPipelineState:_rgbPipeline];
        [renderEncoder setFragmentTexture:_rgbTexture atIndex:0];
        [renderEncoder setFragmentSamplerState:_sampler atIndex:0];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

        // Right half: Depth
        MTLViewport rightViewport = {halfWidth, 0, halfWidth, drawableSize.height, 0, 1};
        [renderEncoder setViewport:rightViewport];
        [renderEncoder setRenderPipelineState:_depthPipeline];
        [renderEncoder setFragmentTexture:_depthTexture atIndex:0];
        [renderEncoder setFragmentSamplerState:_sampler atIndex:0];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

        [renderEncoder endEncoding];

        // Present drawable
        [commandBuffer presentDrawable:view.currentDrawable];
    }

    // Commit command buffer
    [commandBuffer commit];
}

- (double)getCurrentFps {
    return currentFps_;
}

@end

// Application delegate
@interface AppDelegate : NSObject <NSApplicationDelegate> {
    std::shared_ptr<kinect_xr::KinectDevice> kinectDevice_;
    std::atomic<uint64_t> videoFrameCount_;
    std::atomic<uint64_t> depthFrameCount_;
}
@property (strong, nonatomic) NSWindow *window;
@property (strong, nonatomic) Renderer *renderer;
@end

@implementation AppDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        videoFrameCount_ = 0;
        depthFrameCount_ = 0;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Initialize Kinect device
    kinectDevice_ = std::make_shared<kinect_xr::KinectDevice>();

    // Check for Kinect
    int deviceCount = kinect_xr::KinectDevice::getDeviceCount();
    if (deviceCount == 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"No Kinect device found"];
        [alert setInformativeText:@"Please connect a Kinect device and restart."];
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }

    std::cout << "Found " << deviceCount << " Kinect device(s)" << std::endl;

    // Initialize device
    kinect_xr::DeviceConfig config;
    config.enableRGB = true;
    config.enableDepth = true;
    config.enableMotor = false;  // Don't need motor for this spike

    kinect_xr::DeviceError error = kinectDevice_->initialize(config);
    if (error != kinect_xr::DeviceError::None) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Failed to initialize Kinect"];
        [alert setInformativeText:[NSString stringWithFormat:@"Error: %s",
                                   kinect_xr::errorToString(error).c_str()]];
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }

    std::cout << "Kinect initialized successfully" << std::endl;

    // Note: Callbacks will be set after renderer is created

    // Start streaming (will set callbacks after renderer creation)
    error = kinectDevice_->startStreams();
    if (error != kinect_xr::DeviceError::None) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Failed to start streams"];
        [alert setInformativeText:[NSString stringWithFormat:@"Error: %s",
                                   kinect_xr::errorToString(error).c_str()]];
        [alert runModal];
        [NSApp terminate:nil];
        return;
    }

    std::cout << "Streams started" << std::endl;
    // Create window
    NSRect frame = NSMakeRect(0, 0, 1280, 480);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:styleMask
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"Metal Kinect Viewer"];
    [_window center];

    // Create Metal device and view
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }

    MTKView *mtkView = [[MTKView alloc] initWithFrame:frame device:device];
    mtkView.clearColor = MTLClearColorMake(0.0, 0.5, 1.0, 1.0); // Blue background

    // Create renderer
    _renderer = [[Renderer alloc] initWithMetalKitView:mtkView];
    if (!_renderer) {
        NSLog(@"Failed to initialize renderer");
        [NSApp terminate:nil];
        return;
    }
    mtkView.delegate = _renderer;

    // Set up Kinect callbacks now that renderer exists
    AppDelegate* __unsafe_unretained selfPtr = self;
    kinectDevice_->setVideoCallback([selfPtr](const void* rgb, uint32_t timestamp) {
        selfPtr->videoFrameCount_++;
        if (selfPtr->videoFrameCount_ % 30 == 0) {
            std::cout << "Video frame " << selfPtr->videoFrameCount_.load()
                     << " at " << timestamp << std::endl;
        }
        // Update texture
        [selfPtr.renderer updateRGBTextureWithData:rgb];
    });

    kinectDevice_->setDepthCallback([selfPtr](const void* depth, uint32_t timestamp) {
        selfPtr->depthFrameCount_++;
        if (selfPtr->depthFrameCount_ % 30 == 0) {
            std::cout << "Depth frame " << selfPtr->depthFrameCount_.load()
                     << " at " << timestamp << std::endl;
        }
        // Update depth texture
        [selfPtr.renderer updateDepthTextureWithData:depth];
    });

    // Set view as window content
    [_window setContentView:mtkView];
    [_window makeKeyAndOrderFront:nil];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    if (kinectDevice_ && kinectDevice_->isStreaming()) {
        std::cout << "Stopping streams..." << std::endl;
        kinectDevice_->stopStreams();
    }
    std::cout << "Total frames - Video: " << videoFrameCount_.load()
             << ", Depth: " << depthFrameCount_.load() << std::endl;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
