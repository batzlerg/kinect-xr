# Kinect XR

An OpenXR runtime implementation for Kinect 1 hardware.

### Current Status

Still in the initial setup phase...currently working on:

- Implementing basic Kinect device management via libfreenect
- Creating foundational abstractions for the Kinect hardware interface

## Project Goal

This project aims to create a standards-compliant OpenXR runtime for Kinect 1 hardware, leveraging the excellent [libfreenect](https://github.com/OpenKinect/libfreenect) project.

This is done with two goals in mind:
- allowing old Kinect hardware to be used for new AR/VR applications
- creating a path to web integration through the (very new) W3C [WebXR candidate proposal](https://www.w3.org/TR/webxr/)

It is also motivated by laziness, because I bought an old Kinect and want to use it in P5.js, but I'm too lazy to write Processing in other languages. So instead, I'm doing:

```
Hardware:   Kinect Device
               ↓
Driver:     libfreenect (for isochronous USB transfer)
               ↓
Runtime:    OpenXR Runtime (Kinect XR)
               ↓
Bridge:     WebSocket application leveraging Kinect XR
               ↓
Browser:    WebXR-compatible Applications
```

It may take me until the WebXR proposal is approved and implemented in 3025 to finish this project, but if not there's a cool ([polyfill](https://github.com/immersive-web/webxr-polyfill)) lib.

## Building the project

First time setup:
```
cmake -B build -S .
```

Build:
```
cmake --build build
```

## Troubleshooting

If you encounter issues on Mac with Google Test during build, you should verify that you don't have `googletest` installed via Homebrew. I ran into issues with this during initial testing setup, and resolved with `brew uninstall googletest`.

If you need to keep Google Test installed via Homebrew for other projects, you will need to configure CMake to ignore the system installation and use the bundled version. I didn't need it for other purposes, so I didn't go all the way down that rabbit hole.