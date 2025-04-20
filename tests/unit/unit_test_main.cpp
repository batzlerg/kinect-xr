#include <gtest/gtest.h>

#include <iostream>

/**
 * @brief Custom main for unit tests
 */
int main(int argc, char** argv) {
  std::cout << "Running Kinect XR Unit Tests (No hardware required)" << std::endl;
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
