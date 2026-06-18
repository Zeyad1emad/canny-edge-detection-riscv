#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <iostream>
#include "../src/gaussian_blur.h"
#include "../src/gaussian_blur_rvv.h"

// Test 1: RVV Equivalence Test (Crucial for Phase 6)
// This test ensures the RVV implementation produces the exact same output 
// (within +/- 1 tolerance due to fixed-point division) as the scalar 2D code.
TEST(GaussianBlurRVVTest, EquivalenceWithScalar) {
    const int W = 100, H = 100;
    std::vector<uint8_t> input(W * H);
    std::vector<uint8_t> outScalar(W * H, 0);
    std::vector<uint8_t> outRVV(W * H, 0);

    // Fill the image with random noise
    for (int i = 0; i < W * H; ++i) {
        input[i] = static_cast<uint8_t>(rand() % 256);
    }

    // Run both versions
    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), outScalar.data(), W, H);
    gaussian_blur_2d_rvv(input.data(), outRVV.data(), W, H);

    // Check interior pixels (RVV implementation skips the 2-pixel border)
    for (int y = 2; y < H - 2; ++y) {
        for (int x = 2; x < W - 2; ++x) {
            // EXPECT_NEAR with tolerance 1 because:
            // Scalar uses (/ 273) and RVV uses (* 240 >> 16)
            EXPECT_NEAR(outScalar[y * W + x], outRVV[y * W + x], 1);
        }
    }
}

// Test 2: Checks if a flat, grey image remains the same after RVV blurring
TEST(GaussianBlurRVVTest, UniformImageInvariant) {
    const int W = 20, H = 20;
    std::vector<uint8_t> input(W * H, 128), output(W * H, 0);

    gaussian_blur_2d_rvv(input.data(), output.data(), W, H);

    for (int y = 2; y < H - 2; ++y) {
        for (int x = 2; x < W - 2; ++x) {
            EXPECT_NEAR(output[y * W + x], 128, 1);
        }
    }
}

// Test 3: If the input is pitch black, the output must be pitch black
TEST(GaussianBlurRVVTest, BlackImageStaysBlack) {
    const int W = 32, H = 32;
    std::vector<uint8_t> input(W * H, 0), output(W * H, 0);

    gaussian_blur_2d_rvv(input.data(), output.data(), W, H);

    for (int y = 2; y < H - 2; ++y) {
        for (int x = 2; x < W - 2; ++x) {
            EXPECT_EQ(output[y * W + x], 0);
        }
    }
}

// Test 4: Test an image size that isn't a multiple of standard vector lengths
// Ensures strip-mining (__riscv_vsetvl_e8m1) handles loop tails correctly
TEST(GaussianBlurRVVTest, NonPowerOfTwoSize) {
    const int W = 47, H = 53; // Odd dimensions
    std::vector<uint8_t> input(W * H, 100), output(W * H, 0);

    gaussian_blur_2d_rvv(input.data(), output.data(), W, H);

    EXPECT_NEAR(output[25 * W + 25], 100, 1);
}

// Test 5: Performance Benchmarking (Scalar vs RVV)
// RVV should theoretically be significantly faster than the scalar 2D approach
TEST(GaussianBlurRVVTest, RVVFasterThanScalar) {
    const int W = 1024, H = 1024; 
    std::vector<uint8_t> input(W * H, 128), outScalar(W * H, 0), outRVV(W * H, 0);

    std::cout << "\n[--- Performance Benchmarking (1024x1024) ---]" << std::endl;

    // Time Scalar 2D
    auto startScalar = std::chrono::high_resolution_clock::now();
    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), outScalar.data(), W, H);
    auto endScalar = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> timeScalar = endScalar - startScalar;

    // Time RVV
    auto startRVV = std::chrono::high_resolution_clock::now();
    gaussian_blur_2d_rvv(input.data(), outRVV.data(), W, H);
    auto endRVV = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> timeRVV = endRVV - startRVV;

    std::cout << "Scalar 2D:  " << timeScalar.count()  << " ms\n";
    std::cout << "RVV Vector: " << timeRVV.count() << " ms\n";
    std::cout << "RVV Speedup: " << (timeScalar.count() / timeRVV.count()) << "x\n";
    std::cout << "[-------------------------------------------]\n" << std::endl;

    // Note: On an emulator like QEMU, the speedup might not be accurate.
    // We do not enforce EXPECT_GT here to avoid test failure on slow emulators.
}