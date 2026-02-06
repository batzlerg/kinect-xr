#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <kinect_xr/device.h>
#include <iostream>
#include <atomic>

// Simple Metal renderer that displays solid color
@interface Renderer : NSObject <MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@end

@implementation Renderer

- (instancetype)initWithMetalKitView:(MTKView *)mtkView {
    self = [super init];
    if (self) {
        _device = mtkView.device;
        _commandQueue = [_device newCommandQueue];

        // Load shaders and create pipeline
        NSError *error = nil;
        id<MTLLibrary> library = [_device newDefaultLibrary];
        if (!library) {
            NSLog(@"Failed to create Metal library");
            return nil;
        }

        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];

        MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDescriptor.vertexFunction = vertexFunction;
        pipelineDescriptor.fragmentFunction = fragmentFunction;
        pipelineDescriptor.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat;

        _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (!_pipelineState) {
            NSLog(@"Failed to create pipeline state: %@", error);
            return nil;
        }
    }
    return self;
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

- (void)drawInMTKView:(MTKView *)view {
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

    // Get render pass descriptor from view
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if (renderPassDescriptor) {
        // Create render encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder setRenderPipelineState:_pipelineState];

        // Draw fullscreen triangle (covers entire viewport)
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];

        [renderEncoder endEncoding];

        // Present drawable
        [commandBuffer presentDrawable:view.currentDrawable];
    }

    // Commit command buffer
    [commandBuffer commit];
}

@end

// Application delegate
@interface AppDelegate : NSObject <NSApplicationDelegate> {
    std::shared_ptr<kinect_xr::KinectDevice> kinectDevice_;
    std::atomic<uint64_t> videoFrameCount_;
    std::atomic<uint64_t> depthFrameCount_;
}
@property (strong, nonatomic) NSWindow *window;
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

    // Set up callbacks
    AppDelegate* __unsafe_unretained selfPtr = self;
    kinectDevice_->setVideoCallback([selfPtr](const void* rgb, uint32_t timestamp) {
        selfPtr->videoFrameCount_++;
        if (selfPtr->videoFrameCount_ % 30 == 0) {
            std::cout << "Video frame " << selfPtr->videoFrameCount_.load()
                     << " at " << timestamp << std::endl;
        }
    });

    kinectDevice_->setDepthCallback([selfPtr](const void* depth, uint32_t timestamp) {
        selfPtr->depthFrameCount_++;
        if (selfPtr->depthFrameCount_ % 30 == 0) {
            std::cout << "Depth frame " << selfPtr->depthFrameCount_.load()
                     << " at " << timestamp << std::endl;
        }
    });

    // Start streaming
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
    Renderer *renderer = [[Renderer alloc] initWithMetalKitView:mtkView];
    if (!renderer) {
        NSLog(@"Failed to initialize renderer");
        [NSApp terminate:nil];
        return;
    }
    mtkView.delegate = renderer;

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
