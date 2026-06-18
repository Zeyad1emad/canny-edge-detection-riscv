#include <iostream>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include "../src/gaussian_blur.h"
#include "../src/gaussian_blur_rvv.h"

// Simple check instead of GoogleTest
bool check_near(int val1, int val2, int tolerance, const char* test_name) {
    if (std::abs(val1 - val2) > tolerance) {
        std::cerr << "FAIL: " << test_name << " (Expected " << val1 << " close to " << val2 << ")\n";
        return false;
    }
    return true;
}

int main() {
    std::cout << "========== Running RVV Tests ==========\n";
    
    // 1. Equivalence Test
    const int W = 100, H = 100;
    std::vector<uint8_t> input(W * H);
    std::vector<uint8_t> outScalar(W * H, 0);
    std::vector<uint8_t> outRVV(W * H, 0);

    for (int i = 0; i < W * H; ++i) input[i] = rand() % 256;

    gaussian_blur_2d<uint8_t, int32_t, int16_t>(input.data(), outScalar.data(), W, H);
    gaussian_blur_2d_rvv(input.data(), outRVV.data(), W, H);

    bool passed = true;
    for (int y = 2; y < H - 2; ++y) {
        for (int x = 2; x < W - 2; ++x) {
            if (!check_near(outScalar[y * W + x], outRVV[y * W + x], 1, "EquivalenceTest")) {
                passed = false;
                break;
            }
        }
    }

    if (passed) {
        std::cout << "[ OK ] All RVV Tests Passed Successfully!\n";
        std::cout << "=======================================\n";
        return 0;
    } else {
        std::cout << "=======================================\n";
        return 1;
    }
}