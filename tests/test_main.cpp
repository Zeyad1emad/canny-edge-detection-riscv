#include <gtest/gtest.h>
#include <vector>
#include <numeric>
#include "../src/image_io.h"
#include "../src/gaussian_blur.h"
#include "../src/sobel.h"

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
