#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "../src/sobel.h"
#include "../src/sobel_rvv.h"

// Helper function to print the test result instead of using GTest's EXPECT_EQ
void print_result(const std::string& test_name, bool passed) {
    if (passed) {
        std::cout << "[SUCCESS] " << test_name << "\n";
    } else {
        std::cout << "[FAILED]  " << test_name << "\n";
    }
}

int main() {
    std::cout << "==========================================\n";
    std::cout << " Running Comprehensive RVV Tests on QEMU \n";
    std::cout << "==========================================\n\n";

    bool all_passed = true;

    // =========================================================================
    // TEST 1: Mandatory Equivalence Test (Random Image)
    // =========================================================================
    {
        const int w = 128, h = 128;
        std::vector<uint8_t> input(w * h);
        
        // Fill the input image with random noise to test all possible kernel values
        for (int i = 0; i < w * h; ++i) {
            input[i] = std::rand() % 256;
        }

        std::vector<int16_t> Gx_s(w * h, 0), Gy_s(w * h, 0);
        std::vector<int16_t> Gx_v(w * h, 0), Gy_v(w * h, 0);

        // Run both the scalar baseline and the RVV implementation
        compute_sobel(input.data(), Gx_s.data(), Gy_s.data(), w, h);
        sobel_rvv(input.data(), Gx_v.data(), Gy_v.data(), w, h);

        bool passed = true;
        // Verify equivalence: outputs must match within a tolerance of +/- 1
        for (int i = 0; i < w * h; ++i) {
            if (std::abs(Gx_s[i] - Gx_v[i]) > 1 || std::abs(Gy_s[i] - Gy_v[i]) > 1) {
                passed = false; 
                break;
            }
        }
        print_result("Test 1: EquivalenceWithScalarBaseline", passed);
        if (!passed) all_passed = false;
    }

    // =========================================================================
    // TEST 2: Uniform Image (Zero Gradients)
    // =========================================================================
    {
        const int w = 6, h = 6;
        // Flat background image (value 128 everywhere)
        std::vector<uint8_t> input(w * h, 128);
        std::vector<int16_t> Gx(w * h, -1), Gy(w * h, -1);

        sobel_rvv(input.data(), Gx.data(), Gy.data(), w, h);

        bool passed = true;
        // Check only the inner pixels that the Sobel filter actually processes
        for (int y = 1; y < h - 1; ++y) {
            for (int x = 1; x < w - 1; ++x) {
                if (Gx[y * w + x] != 0 || Gy[y * w + x] != 0) { 
                    passed = false; 
                    break; 
                }
            }
        }
        print_result("Test 2: UniformImageProducesZeroGradients", passed);
        if (!passed) all_passed = false;
    }

    // =========================================================================
    // TEST 3: Vertical Edge Detection
    // =========================================================================
    {
        const int w = 6, h = 6;
        std::vector<uint8_t> input(w * h, 0);
        
        // Left half is black (0), right half is white (255)
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                input[y * w + x] = (x < 3) ? 0 : 255;
            }
        }

        std::vector<int16_t> Gx(w * h, 0), Gy(w * h, 0);
        sobel_rvv(input.data(), Gx.data(), Gy.data(), w, h);

        // Check an internal pixel directly on the edge line, and ensure border remains 0
        bool passed = (Gx[2 * w + 2] == 1020 && Gy[2 * w + 2] == 0 && Gx[0] == 0);
        print_result("Test 3: VerticalEdgeDetectsGxAndZerosGy", passed);
        if (!passed) all_passed = false;
    }

    // =========================================================================
    // TEST 4: Horizontal Edge Detection
    // =========================================================================
    {
        const int w = 6, h = 6;
        std::vector<uint8_t> input(w * h, 0);
        
        // Top half is black (0), bottom half is white (255)
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                input[y * w + x] = (y < 3) ? 0 : 255;
            }
        }

        std::vector<int16_t> Gx(w * h, 0), Gy(w * h, 0);
        sobel_rvv(input.data(), Gx.data(), Gy.data(), w, h);

        // Check an internal pixel directly on the horizontal edge line
        bool passed = (Gx[2 * w + 2] == 0 && Gy[2 * w + 2] == 1020);
        print_result("Test 4: HorizontalEdgeDetectsGyAndZerosGx", passed);
        if (!passed) all_passed = false;
    }

    std::cout << "\n==========================================\n";
    if (all_passed) {
        std::cout << " ALL TESTS PASSED SUCCESSFULLY! \n";
    } else {
        std::cout << " SOME TESTS FAILED. CHECK THE OUTPUT! \n";
    }
    std::cout << "==========================================\n";

    return all_passed ? 0 : 1;
}