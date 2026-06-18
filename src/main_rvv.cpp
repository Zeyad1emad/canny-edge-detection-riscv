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

inline uint64_t read_cycles() {
    uint64_t cycles;
#ifdef __riscv
    asm volatile("rdcycle %0" : "=r"(cycles));
#else
    cycles = 0; // Fallback to avoid errors if compiled on host by mistake
#endif
    return cycles;
}

double get_time_diff_ms(uint64_t start, uint64_t end) {
    // Assuming roughly 100MHz clock for Bare-metal QEMU user-mode profiling
    return static_cast<double>(end - start) / 100000.0; 
}

int main(int argc, char** argv) {
    // Parameters only: width, height, low, high (No filenames needed)
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <width> <height> [low_threshold] [high_threshold]\n";
        return 1;
    }

    int width = std::stoi(argv[1]);
    int height = std::stoi(argv[2]);
    uint8_t low_thresh = (argc > 3) ? static_cast<uint8_t>(std::stoi(argv[3])) : 50;
    uint8_t high_thresh = (argc > 4) ? static_cast<uint8_t>(std::stoi(argv[4])) : 150;

    if (width <= 0 || height <= 0) {
        std::cerr << "Error: Width and height must be positive.\n";
        return 1;
    }

    size_t image_size = static_cast<size_t>(width) * height;

    std::cerr << "[*] RVV Canny Edge Pipeline starting...\n";
    std::cerr << "[*] Loading image (" << width << "x" << height << ") from STDIN...\n";
    
    // Pass dummy names, because image_io.cpp (with #ifdef __riscv) will ignore them and read from stdin.
    uint8_t* input_image_buffer = load_raw_image("dummy_in.raw", width, height);
    if (!input_image_buffer) {
        std::cerr << "Error: Failed to load input image data!\n";
        return 1;
    }

    std::vector<uint8_t> blurred(image_size);
    std::vector<int16_t> Gx(image_size), Gy(image_size);
    std::vector<uint8_t> magnitude(image_size);
    std::vector<float> angle(image_size);
    std::vector<uint8_t> nms(image_size);
    std::vector<uint8_t> final_edges(image_size);

    double total_gaussian = 0.0, total_sobel = 0.0, total_mag_angle = 0.0, total_nms = 0.0, total_hysteresis = 0.0;
    
    // Change this to 100 or 200 for a more accurate average profiling
    const int NUM_ITERATIONS = 1; 
    std::cerr << "[*] Executing functions (" << NUM_ITERATIONS << " iterations)...\n";

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        uint64_t t0 = read_cycles();
        gaussian_blur_2d_rvv(input_image_buffer, blurred.data(), width, height);
        
        uint64_t t1 = read_cycles();
        sobel_rvv(blurred.data(), Gx.data(), Gy.data(), width, height);
        
        uint64_t t2 = read_cycles();
        compute_magnitude_rvv(Gx.data(), Gy.data(), magnitude.data(), width, height);
        
        for (size_t j = 0; j < image_size; ++j) {
            angle[j] = std::atan2(static_cast<float>(Gy[j]), static_cast<float>(Gx[j]));
        }
        
        uint64_t t3 = read_cycles();
        non_maximum_suppression(magnitude.data(), angle.data(), nms.data(), width, height);
        
        uint64_t t4 = read_cycles();
        apply_thresholding(nms.data(), final_edges.data(), width, height, low_thresh, high_thresh);
        uint64_t t5 = read_cycles();

        total_gaussian += get_time_diff_ms(t0, t1);
        total_sobel += get_time_diff_ms(t1, t2);
        total_mag_angle += get_time_diff_ms(t2, t3);
        total_nms += get_time_diff_ms(t3, t4);
        total_hysteresis += get_time_diff_ms(t4, t5);
    }

    std::cerr << "[*] Processing complete. Saving edges to STDOUT...\n";
    // Pass dummy name, because image_io.cpp will output to stdout directly.
    save_raw_image("dummy_out.raw", final_edges.data(), width, height);

    double avg_gaussian = total_gaussian / NUM_ITERATIONS;
    double avg_sobel = total_sobel / NUM_ITERATIONS;
    double avg_mag_angle = total_mag_angle / NUM_ITERATIONS;
    double avg_nms = total_nms / NUM_ITERATIONS;
    double avg_hysteresis = total_hysteresis / NUM_ITERATIONS;
    double total_avg_time = avg_gaussian + avg_sobel + avg_mag_angle + avg_nms + avg_hysteresis;

    std::cerr << "\n========================================================\n";
    std::cerr << "          HYBRID RVV PIPELINE PROFILING REPORT          \n";
    std::cerr << "========================================================\n";
    std::cerr << std::fixed << std::setprecision(3);
    std::cerr << "1. Gaussian Blur (RVV): " << std::setw(8) << avg_gaussian << " ms\n";
    std::cerr << "2. Sobel Operator(RVV): " << std::setw(8) << avg_sobel << " ms\n";
    std::cerr << "3. Mag(RVV)+Angle(Scl): " << std::setw(8) << avg_mag_angle << " ms\n";
    std::cerr << "4. NMS (Scalar)       : " << std::setw(8) << avg_nms << " ms\n";
    std::cerr << "5. Hysteresis (Scalar): " << std::setw(8) << avg_hysteresis << " ms\n";
    std::cerr << "--------------------------------------------------------\n";
    std::cerr << "TOTAL TIME            : " << std::setw(8) << total_avg_time << " ms\n";
    std::cerr << "========================================================\n";

    free(input_image_buffer);
    return 0;
}