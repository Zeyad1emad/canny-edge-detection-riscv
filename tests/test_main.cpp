#include <gtest/gtest.h>
#include <vector>
#include <numeric>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../src/image_io.h"
#include "../src/gaussian_blur.h"
#include "../src/sobel.h"
#include "../src/magnitude.h"
#include "../src/direction.h"

// Test Gaussian Blur on a constant image
TEST(GaussianBlurTest, ConstantImage) {
    const int W = 10, H = 10;
    std::vector<uint8_t> input(W * H, 100);
    std::vector<uint8_t> output(W * H, 0);

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), output.data(), W, H);

    for (int y = 2; y < H - 2; ++y) {
        for (int x = 2; x < W - 2; ++x) {
            EXPECT_NEAR(output[y * W + x], 100, 2);
        }
    }
}

// Test separable Gaussian blur matches 2D Gaussian blur
TEST(GaussianBlurTest, SeparableMatches2D) {
    const int W = 20, H = 20;
    std::vector<uint8_t> input(W * H);
    std::iota(input.begin(), input.end(), 0);
    
    std::vector<uint8_t> out2d(W * H, 0);
    std::vector<uint8_t> outSep(W * H, 0);

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), out2d.data(), W, H);
    gaussian_blur_separable<uint8_t, int32_t, int16_t>(input.data(), outSep.data(), W, H);

    for (int i = 0; i < W * H; ++i) {
        EXPECT_NEAR(out2d[i], outSep[i], 5);
    }
}

// Test Sobel operator on a vertical edge
TEST(SobelTest, VerticalEdge) {
    const int W = 10, H = 10;
    std::vector<uint8_t> input(W * H, 0);
    for (int y = 0; y < H; ++y) {
        for (int x = W / 2; x < W; ++x) {
            input[y * W + x] = 255;
        }
    }

    std::vector<int16_t> Gx(W * H), Gy(W * H);
    compute_sobel(input.data(), Gx.data(), Gy.data(), W, H);

    int edge_x = W / 2;
    EXPECT_GT(std::abs(Gx[5 * W + edge_x]), 0);
    EXPECT_EQ(Gy[5 * W + edge_x], 0);
}

// Compare performance and verify speedup of Separable vs 2D Blur
TEST(PerformanceTest, BlurSpeedup) {
    const int W = 1048, H = 1048; 
    std::vector<uint8_t> input(W * H, 128);
    std::vector<uint8_t> out2d(W * H, 0);
    std::vector<uint8_t> outSep(W * H, 0);

    auto start_2d = std::chrono::high_resolution_clock::now();
    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), out2d.data(), W, H);
    auto end_2d = std::chrono::high_resolution_clock::now();

    auto start_sep = std::chrono::high_resolution_clock::now();
    gaussian_blur_separable<uint8_t, int32_t, int16_t>(input.data(), outSep.data(), W, H);
    auto end_sep = std::chrono::high_resolution_clock::now();

    auto time_2d = std::chrono::duration_cast<std::chrono::milliseconds>(end_2d - start_2d).count();
    auto time_sep = std::chrono::duration_cast<std::chrono::milliseconds>(end_sep - start_sep).count();

    std::cout << "[ PERFORMANCE ] 2D Blur execution time: " << time_2d << " ms" << std::endl;
    std::cout << "[ PERFORMANCE ] Separable Blur execution time: " << time_sep << " ms" << std::endl;
    
    EXPECT_LE(time_sep, time_2d) << "Optimized Separable Blur should be faster than standard 2D Blur.";
}

// Verify that optimized cross-multiplication direction matches traditional atan2 sectors
TEST(DirectionTest, IntegerCrossMultiplicationMatchesAtan2) {
    int16_t Gx = 100;
    int16_t Gy = 30; // Approx ##### degrees (Sector 0 threshold is 22.5)

    float angle = std::atan2(static_cast<float>(Gy), static_cast<float>(Gx)) * (180.0f / 3.14159265f);
    if (angle < 0) angle += 180.0f;
    
    uint8_t expected_dir;
    if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle <= 180)) expected_dir = 0;
    else if (angle >= 22.5 && angle < 67.5) expected_dir = 1;
    else if (angle >= 67.5 && angle < 112.5) expected_dir = 2;
    else expected_dir = 3;

    std::vector<int16_t> mock_Gx = { Gx };
    std::vector<int16_t> mock_Gy = { Gy };
    std::vector<uint8_t> actual_dir(1);

    compute_direction(mock_Gx.data(), mock_Gy.data(), actual_dir.data(), 1, 1);

    EXPECT_EQ(actual_dir[0], expected_dir) << "Integer cross-multiplication branch mismatch with atan2 trigonometry.";
}

// Ensure the normalization stage securely handles potential division by zero
TEST(RobustnessTest, HandlesPureBlackZeroMaxMagnitude) {
    const int W = 16, H = 16;
    std::vector<int16_t> zero_Gx(W * H, 0);
    std::vector<int16_t> zero_Gy(W * H, 0);
    std::vector<uint8_t> out_l1(W * H, 255);
    std::vector<uint8_t> out_l2(W * H, 255);

    EXPECT_NO_THROW({
        compute_magnitude_l1(zero_Gx.data(), zero_Gy.data(), out_l1.data(), W, H);
        compute_magnitude_l2(zero_Gx.data(), zero_Gy.data(), out_l2.data(), W, H);
    });

    for (int i = 0; i < W * H; ++i) {
        EXPECT_EQ(out_l1[i], 0) << "L1 output should be clamped to 0 on a uniform black grid.";
        EXPECT_EQ(out_l2[i], 0) << "L2 output should be clamped to 0 on a uniform black grid.";
    }
}

// Verify mathematical correlation and normalized trends between L1 and L2 Norms
TEST(MagnitudeTest, L1MatchesL2DistributionTrend) {
    const int W = 32, H = 32;
    std::vector<int16_t> Gx(W * H, 80);
    std::vector<int16_t> Gy(W * H, 45);
    std::vector<uint8_t> out_l1(W * H, 0);
    std::vector<uint8_t> out_l2(W * H, 0);

    compute_magnitude_l1(Gx.data(), Gy.data(), out_l1.data(), W, H);
    compute_magnitude_l2(Gx.data(), Gy.data(), out_l2.data(), W, H);

    for (int i = 0; i < W * H; ++i) {
        EXPECT_NEAR(out_l1[i], out_l2[i], 15) << "Normalized L1 and L2 variants vary beyond the functional threshold.";
    }
}

// Test Canny pipeline with a generated circle pattern
TEST(CannyPipelineTest, CirclePatternEdges) {
    const int W = 100, H = 100;
    size_t image_size = static_cast<size_t>(W) * H;
    size_t aligned_size = ((image_size + 63) / 64) * 64;
    uint8_t* input_image_buffer = static_cast<uint8_t*>(aligned_alloc(64, aligned_size));
    ASSERT_NE(input_image_buffer, nullptr) << "Failed to allocate memory for input image.";

    test_image_generator(input_image_buffer, W, H, static_cast<int>(Pattern::CIRCLE));

    std::vector<uint8_t> blurred(image_size);
    std::vector<int16_t> Gx(image_size), Gy(image_size);
    std::vector<uint8_t> magnitude(image_size);
    std::vector<float> angle(image_size);
    std::vector<uint8_t> nms(image_size);
    std::vector<uint8_t> final_edges(image_size);

    gaussian_blur_separable<uint8_t, int32_t, int16_t>(input_image_buffer, blurred.data(), W, H);
    compute_sobel(blurred.data(), Gx.data(), Gy.data(), W, H);
    compute_magnitude_angle(Gx.data(), Gy.data(), magnitude.data(), angle.data(), W, H);
    non_maximum_suppression(magnitude.data(), angle.data(), nms.data(), W, H);
    apply_thresholding(nms.data(), final_edges.data(), W, H, 50, 150);

    int edge_count = 0;
    for (uint8_t p : final_edges) {
        if (p > 0) {
            edge_count++;
        }
    }
    EXPECT_GT(edge_count, 0) << "No edges detected in the circle pattern.";

    free(input_image_buffer);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}