#include "gtest/gtest.h"
#include "../src/sobel.h"
#include <vector>
#include <cstdint>
#include <cmath>

// =========================================================================
// CORE TEST 1: Uniform Image (Flat Background) - Size 6x6
// =========================================================================
TEST(SobelPipelineTest, UniformImageProducesZeroGradients) {
    const int width = 6;
    const int height = 6;
    
    std::vector<uint8_t> input(width * height, 128);
    std::vector<int16_t> Gx(width * height, -1); 
    std::vector<int16_t> Gy(width * height, -1);

    compute_sobel(input.data(), Gx.data(), Gy.data(), width, height);

    for (int i = 0; i < width * height; ++i) {
        EXPECT_EQ(Gx[i], 0);
        EXPECT_EQ(Gy[i], 0);
    }
}

// =========================================================================
// CORE TEST 2: Sharp Vertical Edge Detection - Size 6x6
// =========================================================================
TEST(SobelPipelineTest, VerticalEdgeDetectsGxAndZerosGy) {
    const int width = 6;
    const int height = 6;
    std::vector<uint8_t> input(width * height, 0);

    // Left half (columns 0,1,2) is Black (0), Right half (columns 3,4,5) is White (255)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            input[y * width + x] = (x < 3) ? 0 : 255;
        }
    }

    std::vector<int16_t> Gx(width * height, 0);
    std::vector<int16_t> Gy(width * height, 0);

    compute_sobel(input.data(), Gx.data(), Gy.data(), width, height);

    // Verify a perfectly internal pixel on the edge line (e.g., x=2, y=2)
    // Sobel-X should detect the huge vertical jump, Sobel-Y should be 0
    EXPECT_EQ(Gx[2 * width + 2], 1020);
    EXPECT_EQ(Gy[2 * width + 2], 0);

    // Verify border pixels remain strictly 0 as required
    EXPECT_EQ(Gx[0 * width + 0], 0);
    EXPECT_EQ(Gy[0 * width + 0], 0);
}

// =========================================================================
// CORE TEST 3: Sharp Horizontal Edge Detection - Size 6x6
// =========================================================================
TEST(SobelPipelineTest, HorizontalEdgeDetectsGyAndZerosGx) {
    const int width = 6;
    const int height = 6;
    std::vector<uint8_t> input(width * height, 0);

    // Top half (rows 0,1,2) is Black (0), Bottom half (rows 3,4,5) is White (255)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            input[y * width + x] = (y < 3) ? 0 : 255;
        }
    }

    std::vector<int16_t> Gx(width * height, 0);
    std::vector<int16_t> Gy(width * height, 0);

    compute_sobel(input.data(), Gx.data(), Gy.data(), width, height);

    // Verify a perfectly internal pixel on the horizontal edge line (e.g., x=2, y=2)
    EXPECT_EQ(Gx[2 * width + 2], 0);
    EXPECT_EQ(Gy[2 * width + 2], 1020);
}

// =========================================================================
// CORE TEST 4: Diagonal Edge Detection - Size 6x6
// =========================================================================
TEST(SobelPipelineTest, DiagonalEdgeProducesSignificantGxAndGy) {
    const int width = 6;
    const int height = 6;
    std::vector<uint8_t> input(width * height, 0);

    // Creating a clean diagonal stepped pattern
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x + y >= 5) {
                input[y * width + x] = 255;
            }
        }
    }

    std::vector<int16_t> Gx(width * height, 0);
    std::vector<int16_t> Gy(width * height, 0);

    compute_sobel(input.data(), Gx.data(), Gy.data(), width, height);

    // An internal pixel sitting right on the diagonal step (e.g., x=2, y=3)
    // must have significant non-zero values in both directions
    EXPECT_NE(Gx[3 * width + 2], 0);
    EXPECT_NE(Gy[3 * width + 2], 0);
}

// =========================================================================
// BONUS TEST 5: Magnitude and Angle Computation - Size 2x2 (Kept small for exact math)
// =========================================================================
TEST(SobelPipelineTest, MagnitudeAndAngleComputationIsAccurate) {
    const int width = 2;
    const int height = 2;

    std::vector<int16_t> Gx(width * height, 30);
    std::vector<int16_t> Gy(width * height, 40);

    std::vector<uint8_t> magnitude(width * height, 0);
    std::vector<float> angle(width * height, 0.0f);

    compute_magnitude_angle(Gx.data(), Gy.data(), magnitude.data(), angle.data(), width, height);

    EXPECT_EQ(magnitude[0], 50); // sqrt(30^2 + 40^2) = 50
    EXPECT_NEAR(angle[0], 53.13f, 0.1f);
}

// =========================================================================
// BONUS TEST 6: Non-Maximum Suppression (Edge Thinning) - Size 5x5
// =========================================================================
TEST(SobelPipelineTest, NonMaximumSuppressionThinsEdges) {
    const int width = 5;
    const int height = 5;

    std::vector<uint8_t> magnitude(width * height, 0);
    // Create a vertical ridge profile in the middle column (x=2)
    for (int y = 1; y < height - 1; ++y) {
        magnitude[y * width + 1] = 100; // Left neighbor
        magnitude[y * width + 2] = 200; // Peak peak (The edge)
        magnitude[y * width + 3] = 100; // Right neighbor
    }
    
    std::vector<float> angle(width * height, 0.0f); // 0 degrees = Horizontal gradient direction
    std::vector<uint8_t> nms_output(width * height, 0);

    non_maximum_suppression(magnitude.data(), angle.data(), nms_output.data(), width, height);

    // The peak should remain intact, neighbors should be suppressed to 0
    EXPECT_EQ(nms_output[2 * width + 2], 200);
    EXPECT_EQ(nms_output[2 * width + 1], 0);
    EXPECT_EQ(nms_output[2 * width + 3], 0);
}

// =========================================================================
// BONUS TEST 7: Double Thresholding and Hysteresis (Robust Validation) - Size 5x5
// =========================================================================
TEST(SobelPipelineTest, HysteresisFiltersNoiseAndKeepsConnectedEdges) {
    const int width = 5;
    const int height = 5;
    std::vector<uint8_t> nms_output(width * height, 0);

    // Low threshold = 50, High threshold = 150
    // Let's create a clear structure away from the outermost border:
    nms_output[2 * width + 2] = 200; // (2,2) -> Strong edge (> 150)
    nms_output[2 * width + 3] = 100; // (2,3) -> Weak edge, directly connected to (2,2)
    nms_output[4 * width + 4] = 100; // (4,4) -> Weak edge, completely isolated at the corner

    std::vector<uint8_t> final_edges(width * height, 0);

    apply_thresholding(nms_output.data(), final_edges.data(), width, height, 50, 150);

    EXPECT_EQ(final_edges[2 * width + 2], 255); // Strong edge must stay 255
    EXPECT_EQ(final_edges[2 * width + 3], 255); // Connected weak edge must be successfully upgraded!
    EXPECT_EQ(final_edges[4 * width + 4], 0);   // Isolated noise must be completely killed
}