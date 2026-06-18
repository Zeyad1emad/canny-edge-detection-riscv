#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include "sobel.h"       
#include "sobel_rvv.h"   

int main() {
    const int width = 1024, height = 1024;
    size_t image_size = width * height;

    // Allocate memory
    std::vector<uint8_t> input(image_size, 128); 
    std::vector<int16_t> Gx_scalar(image_size), Gy_scalar(image_size);
    std::vector<int16_t> Gx_rvv(image_size), Gy_rvv(image_size);

    const int ITERATIONS = 100; // قللنا العدد شوية عشان QEMU مياخدش وقت طويل

    // 1. Benchmark Scalar
    auto start_scalar = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < ITERATIONS; ++i) {
        compute_sobel(input.data(), Gx_scalar.data(), Gy_scalar.data(), width, height);
    }
    auto end_scalar = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_scalar = end_scalar - start_scalar;

    // 2. Benchmark RVV
    auto start_rvv = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < ITERATIONS; ++i) {
        sobel_rvv(input.data(), Gx_rvv.data(), Gy_rvv.data(), width, height);
    }
    auto end_rvv = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_rvv = end_rvv - start_rvv;

    // Calculate Averages
    double avg_scalar = time_scalar.count() / ITERATIONS;
    double avg_rvv = time_rvv.count() / ITERATIONS;

    // Print Results
    std::cout << "====================================\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Sobel Scalar: " << avg_scalar << " ms/frame\n";
    std::cout << "Sobel RVV   : " << avg_rvv << " ms/frame\n";
    std::cout << "Speedup     : " << (avg_scalar / avg_rvv) << "x\n";
    std::cout << "====================================\n";

    return 0;
}