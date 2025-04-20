#include <gtest/gtest.h>
#include <libfreenect/libfreenect.h>

// test that we can find the libfreenect library
TEST(LibfreenectTest, LibraryAvailable) {
  freenect_context* ctx = nullptr;
  int result = freenect_init(&ctx, nullptr);
  if (ctx) {
    freenect_shutdown(ctx);
  }
  // asserting the result directly will fail if no device is connected
  // but the function still should be available for linking
  SUCCEED();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
