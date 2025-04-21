#include <libfreenect/libfreenect.h>

#include <iostream>

int main() {
  std::cout << "Kinect XR test program" << std::endl;

  freenect_context* ctx;
  if (freenect_init(&ctx, nullptr) < 0) {
    std::cerr << "Failed to initialize freenect context" << std::endl;
    return 1;
  }

  // get number of devices
  int num_devices = freenect_num_devices(ctx);
  std::cout << "Found " << num_devices << " Kinect device(s)" << std::endl;

  freenect_shutdown(ctx);

  return 0;
}
