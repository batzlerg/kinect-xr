#include <gtest/gtest.h>
#include "kinect_xr/runtime.h"
#include <thread>
#include <chrono>

using namespace kinect_xr;

class FrameCacheTest : public ::testing::Test {
protected:
    FrameCache cache_;
};

// Basic cache operations

TEST_F(FrameCacheTest, InitialState) {
    EXPECT_FALSE(cache_.rgbValid);
    EXPECT_FALSE(cache_.depthValid);
    EXPECT_EQ(cache_.rgbTimestamp, 0u);
    EXPECT_EQ(cache_.depthTimestamp, 0u);
    EXPECT_EQ(cache_.rgbData.size(), 640u * 480u * 3u);
    EXPECT_EQ(cache_.depthData.size(), 640u * 480u);
}

TEST_F(FrameCacheTest, StoreRGBFrame) {
    std::lock_guard<std::mutex> lock(cache_.mutex);

    // Fill with test pattern
    for (size_t i = 0; i < cache_.rgbData.size(); ++i) {
        cache_.rgbData[i] = static_cast<uint8_t>(i % 256);
    }
    cache_.rgbTimestamp = 12345;
    cache_.rgbValid = true;

    EXPECT_TRUE(cache_.rgbValid);
    EXPECT_EQ(cache_.rgbTimestamp, 12345u);
    EXPECT_EQ(cache_.rgbData[0], 0u);
    EXPECT_EQ(cache_.rgbData[255], 255u);
}

TEST_F(FrameCacheTest, StoreDepthFrame) {
    std::lock_guard<std::mutex> lock(cache_.mutex);

    // Fill with test depth values
    for (size_t i = 0; i < cache_.depthData.size(); ++i) {
        cache_.depthData[i] = static_cast<uint16_t>(i % 2048);  // 11-bit values
    }
    cache_.depthTimestamp = 54321;
    cache_.depthValid = true;

    EXPECT_TRUE(cache_.depthValid);
    EXPECT_EQ(cache_.depthTimestamp, 54321u);
    EXPECT_EQ(cache_.depthData[0], 0u);
    EXPECT_EQ(cache_.depthData[2047], 2047u);
}

TEST_F(FrameCacheTest, IndependentRGBandDepth) {
    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        cache_.rgbValid = true;
        cache_.rgbTimestamp = 111;
        cache_.depthValid = false;
        cache_.depthTimestamp = 0;
    }

    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        EXPECT_TRUE(cache_.rgbValid);
        EXPECT_FALSE(cache_.depthValid);
        EXPECT_EQ(cache_.rgbTimestamp, 111u);
        EXPECT_EQ(cache_.depthTimestamp, 0u);
    }
}

// Thread safety tests

TEST_F(FrameCacheTest, ThreadSafeWrite) {
    const int numWrites = 100;

    // Writer thread for RGB
    std::thread rgbWriter([this]() {
        for (int i = 0; i < numWrites; ++i) {
            std::lock_guard<std::mutex> lock(cache_.mutex);
            cache_.rgbTimestamp = i;
            cache_.rgbValid = true;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // Writer thread for depth
    std::thread depthWriter([this]() {
        for (int i = 0; i < numWrites; ++i) {
            std::lock_guard<std::mutex> lock(cache_.mutex);
            cache_.depthTimestamp = i * 2;
            cache_.depthValid = true;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    rgbWriter.join();
    depthWriter.join();

    // Both should be valid after threads complete
    EXPECT_TRUE(cache_.rgbValid);
    EXPECT_TRUE(cache_.depthValid);
    // Timestamps should be from last write
    EXPECT_GE(cache_.rgbTimestamp, 0u);
    EXPECT_GE(cache_.depthTimestamp, 0u);
}

TEST_F(FrameCacheTest, ThreadSafeReadWrite) {
    std::atomic<bool> writerDone{false};
    std::atomic<int> validReads{0};

    // Writer thread
    std::thread writer([this, &writerDone]() {
        for (int i = 0; i < 50; ++i) {
            std::lock_guard<std::mutex> lock(cache_.mutex);
            cache_.rgbTimestamp = i;
            cache_.rgbValid = true;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        writerDone = true;
    });

    // Reader thread
    std::thread reader([this, &writerDone, &validReads]() {
        while (!writerDone) {
            std::lock_guard<std::mutex> lock(cache_.mutex);
            if (cache_.rgbValid) {
                validReads++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    });

    writer.join();
    reader.join();

    // Should have read valid data multiple times
    EXPECT_GT(validReads.load(), 0);
}

// Data integrity tests

TEST_F(FrameCacheTest, RGBDataSize) {
    EXPECT_EQ(cache_.rgbData.size(), 640u * 480u * 3u);  // 921600 bytes
}

TEST_F(FrameCacheTest, DepthDataSize) {
    EXPECT_EQ(cache_.depthData.size(), 640u * 480u);  // 307200 uint16_t
}

TEST_F(FrameCacheTest, OverwriteData) {
    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        cache_.rgbTimestamp = 100;
        cache_.rgbValid = true;
    }

    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        EXPECT_EQ(cache_.rgbTimestamp, 100u);
    }

    // Overwrite
    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        cache_.rgbTimestamp = 200;
        cache_.rgbValid = true;
    }

    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        EXPECT_EQ(cache_.rgbTimestamp, 200u);
    }
}

TEST_F(FrameCacheTest, InvalidateFrames) {
    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        cache_.rgbValid = true;
        cache_.depthValid = true;
    }

    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        cache_.rgbValid = false;
        cache_.depthValid = false;
    }

    {
        std::lock_guard<std::mutex> lock(cache_.mutex);
        EXPECT_FALSE(cache_.rgbValid);
        EXPECT_FALSE(cache_.depthValid);
    }
}
