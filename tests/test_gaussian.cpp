#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <iostream>
#include "../src/gaussian_blur.h"


// Test 1: Checks if a flat, grey image remains the same after blurring
// Since every pixel is the same, the average should theoretically never change
TEST(GaussianBlurTest, UniformImageInvariant) {
    const int W = 20, H = 20;
    std::vector<uint8_t> input(W * H, 128), output(W * H, 0);

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), output.data(), W, H);

    // We only check the interior because the edges don't have enough neighbors to blur properly
    for (int y = 2; y < H - 2; ++y)
        for (int x = 2; x < W - 2; ++x)
            EXPECT_NEAR(output[y * W + x], 128, 1);  // Allowing a tiny difference of 1 due to rounding math
}


// Test 2: If the input is pitch black, the output must be pitch black
// 0 times any kernel value is still 0
TEST(GaussianBlurTest, BlackImageStaysBlack) {
    const int W = 20, H = 20;
    std::vector<uint8_t> input(W * H, 0), output(W * H, 0);

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), output.data(), W, H);

    for (int i = 0; i < W * H; ++i)
        EXPECT_EQ(output[i], 0);
}


// Test 3: Put a single bright pixel in the middle and see if the blur spreads out evenly
// This confirms the kernel is applied symmetrically in all directions
TEST(GaussianBlurTest, ImpulseSpreadsSymmetrically) {
    const int W = 20, H = 20;
    std::vector<uint8_t> input(W * H, 0), output(W * H, 0);
    input[10 * W + 10] = 255; // The "impulse" - one white dot

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), output.data(), W, H);

    EXPECT_GT(output[10 * W + 10], 0);   // The center should still have some brightness
    EXPECT_GT(output[10 * W + 11], 0);   // Right neighbor should now be partially lit
    EXPECT_GT(output[11 * W + 10], 0);   // Bottom neighbor should now be partially lit

    // Check if the light spread exactly the same to the left as it did to the right
    EXPECT_EQ(output[10 * W + 9], output[10 * W + 11]);
    // Check if the light spread exactly the same to the top as it did to the bottom
    EXPECT_EQ(output[9 * W + 10], output[11 * W + 10]);
}

// Test 4: Verify the math for a single pixel using a known expected result
// Using the center weight of the kernel: (255 * 41) / 273 = 38.27... which rounds to 38
TEST(GaussianBlurTest, SinglePointExactValue) {
    const int W = 5, H = 5;
    std::vector<uint8_t> input(W * H, 0), output(W * H, 0);
    input[12] = 255;  // Center pixel of a 5x5 grid

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), output.data(), W, H);

    EXPECT_EQ(output[12], 38);
}


// Test 5: Make sure the 2D version and the Separable version produce nearly identical images
// They should be very close, but might differ slightly due to different rounding steps
TEST(GaussianBlurTest, Compare2DAndSeparable) {
    const int W = 100, H = 100;
    std::vector<uint8_t> input(W * H), out2D(W * H, 0), outSep(W * H, 0);
    // Fill the image with random noise to give the filter something complex to work with
    for (int i = 0; i < W * H; ++i) input[i] = static_cast<uint8_t>(rand() % 256);

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), out2D.data(), W, H);
    gaussian_blur_separable<uint8_t, int32_t, int16_t>(input.data(), outSep.data(), W, H);

    // Skip the borders (2 pixels wide) because the two methods handle edges differently
    for (int y = 2; y < H - 2; ++y)
        for (int x = 2; x < W - 2; ++x)
            EXPECT_NEAR(out2D[y * W + x], outSep[y * W + x], 5);
}


// Test 6: Test an image size that isn't a power of two
// This makes sure the loops don't assume the image fits perfectly into blocks (like 16 or 32)
TEST(GaussianBlurTest, NonPowerOfTwoSize) {
    const int W = 48, H = 48;
    std::vector<uint8_t> input(W * H, 100), output(W * H, 0);

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), output.data(), W, H);

    EXPECT_NEAR(output[24 * W + 24], 100, 1);
}


// Test 7: Race the two functions against each other
// The Separable filter (row pass then column pass) should be much faster than the full 2D loop
TEST(GaussianBlurTest, SeparableFasterThan2D) {
    const int W = 1024, H = 1024; // Use a large 1MP image to make the time difference obvious
    std::vector<uint8_t> input(W * H, 128), out2D(W * H, 0), outSep(W * H, 0);

    std::cout << "\n[--- Performance Benchmarking (1024x1024) ---]" << std::endl;

    // Time the standard 2D Convolution
    auto start2D = std::chrono::high_resolution_clock::now();
    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), out2D.data(), W, H);
    auto end2D = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time2D = end2D - start2D;

    // Time the optimized Separable filter
    auto startSep = std::chrono::high_resolution_clock::now();
    gaussian_blur_separable<uint8_t, int32_t, int16_t>(input.data(), outSep.data(), W, H);
    auto endSep = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> timeSep = endSep - startSep;

    // Print the results to the console so we can see the actual numbers
    std::cout << "2D Convolution:   " << time2D.count()  << " ms\n";
    std::cout << "Separable Filter: " << timeSep.count() << " ms\n";
    std::cout << "Speedup:          " << (time2D.count() / timeSep.count()) << "x\n";
    std::cout << "[-------------------------------------------]\n" << std::endl;

    // Fail the test if the "optimized" version is actually slower than the basic one
    EXPECT_GT(time2D.count(), timeSep.count());
}

