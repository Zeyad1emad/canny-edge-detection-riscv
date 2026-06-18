#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <iomanip>
#include <cmath>

// Core Pipeline Headers
#include "image_io.h"
#include "sobel.h"

// RVV Optimized Headers
#include "gaussian_blur_rvv.h"
#include "sobel_rvv.h"
#include "magnitude_rvv.h"

// Read RISC-V Hardware Cycle Counter
inline uint64_t read_cycles() {
    uint64_t cycles;
    // Reads the 64-bit cycle CSR directly from the hardware
    asm volatile("rdcycle %0" : "=r"(cycles));
    return cycles;
}

// Helper to convert elapsed cycles to milliseconds
// In simulation/QEMU, measuring raw cycles is actually MUCH more accurate 
// for hardware profiling than wall-clock time.
// We normalize by an assumed 100 MHz clock or just return cycles/100000 for standard tracking.
double get_time_diff_ms(uint64_t start, uint64_t end) {
    return static_cast<double>(end - start) / 100000.0; 
}

int main(int argc, char** argv) {
    // Validate required arguments
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <width> <height> <input_raw> <output_raw> [low_threshold] [high_threshold]" << std::endl;
        return 1;
    }

    int width = std::stoi(argv[1]);
    int height = std::stoi(argv[2]);
    const char* input_filename = argv[3];
    const char* output_filename = argv[4];
    
    uint8_t low_thresh = (argc > 5) ? static_cast<uint8_t>(std::stoi(argv[5])) : 50;
    uint8_t high_thresh = (argc > 6) ? static_cast<uint8_t>(std::stoi(argv[6])) : 150;

    if (width <= 0 || height <= 0) {
        std::cerr << "Error: Width and height must be positive." << std::endl;
        return 1;
    }

    size_t image_size = static_cast<size_t>(width) * height;

    // Load real image using custom 64-byte aligned function
    uint8_t* input_image_buffer = load_raw_image(input_filename, width, height);
    if (!input_image_buffer) {
        std::cerr << "Error: Failed to load input image: " << input_filename << std::endl;
        return 1;
    }
    std::cout << "[*] Loaded input image: " << input_filename << " (" << width << "x" << height << ")" << std::endl;

    // Allocate buffers for pipeline stages
    std::vector<uint8_t> blurred(image_size);
    std::vector<int16_t> Gx(image_size), Gy(image_size);
    std::vector<uint8_t> magnitude(image_size);
    std::vector<float> angle(image_size);
    std::vector<uint8_t> nms(image_size);
    std::vector<uint8_t> final_edges(image_size);

    // Profiling accumulators
    double total_gaussian = 0.0;
    double total_sobel = 0.0;
    double total_mag_angle = 0.0;
    double total_nms = 0.0;
    double total_hysteresis = 0.0;
    
    const int NUM_ITERATIONS = 200;
    std::cout << "\n[*] Starting RVV Profiling Sweep (" << NUM_ITERATIONS << " iterations) for stable measurements...\n";

    // Profiling Loop
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        uint64_t t0, t1, t2, t3, t4, t5;

        // Stage 1: Gaussian Blur (RVV Accelerated)
        t0 = read_cycles();
        gaussian_blur_2d_rvv(input_image_buffer, blurred.data(), width, height);
        
        // Stage 2: Sobel Operator (RVV Accelerated)
        t1 = read_cycles();
        sobel_rvv(blurred.data(), Gx.data(), Gy.data(), width, height);
        
        // Stage 3: Magnitude (RVV Accelerated) & Angle (Scalar Fallback)
        t2 = read_cycles();
        compute_magnitude_rvv(Gx.data(), Gy.data(), magnitude.data(), width, height);
        
        // Compute angles in scalar due to complex trignometric nature in hardware vectorizing
        for (size_t j = 0; j < image_size; ++j) {
            angle[j] = std::atan2(static_cast<float>(Gy[j]), static_cast<float>(Gx[j]));
        }

        // Stage 4: Non-Maximum Suppression (Scalar)
        t3 = read_cycles();
        non_maximum_suppression(magnitude.data(), angle.data(), nms.data(), width, height);

        // Stage 5: Hysteresis Thresholding (Scalar)
        t4 = read_cycles();
        apply_thresholding(nms.data(), final_edges.data(), width, height, low_thresh, high_thresh);
        t5 = read_cycles();

        // Accumulate relative "time" based on hardware cycles
        total_gaussian += get_time_diff_ms(t0, t1);
        total_sobel += get_time_diff_ms(t1, t2);
        total_mag_angle += get_time_diff_ms(t2, t3);
        total_nms += get_time_diff_ms(t3, t4);
        total_hysteresis += get_time_diff_ms(t4, t5);
    }

    // Save final processed edges (from the last iteration)
    save_raw_image(output_filename, final_edges.data(), width, height);