#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <iomanip>
#include <chrono> // Reliable cross-platform library for time measurement in C++

// Core Pipeline Headers
#include "image_io.h"
#include "sobel.h"

// RVV Optimized Headers
#include "gaussian_blur_rvv.h"
#include "sobel_rvv.h"
#include "magnitude_rvv.h"
#include "direction_rvv.h"

// New time measurement function using std::chrono
using TimePoint = std::chrono::high_resolution_clock::time_point;
double get_time_diff_ms(TimePoint start, TimePoint end) {
    std::chrono::duration<double, std::milli> diff = end - start;
    return diff.count();
}

// Read CPU Cycles
inline uint64_t read_cycles() {
    uint64_t cycles;
#ifdef __riscv
    asm volatile("rdcycle %0" : "=r"(cycles));
#else
    cycles = 0; 
#endif
    return cycles;
}

extern "C" char _start;
extern "C" char _end;

int main(int argc, char** argv) {
    size_t binary_size = (size_t)(&_end - &_start);
    std::cerr << "[*] Binary Executable Size (In-Memory Image): " << binary_size << " bytes\n";

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
    
    uint8_t* input_image_buffer = load_raw_image("dummy_in.raw", width, height);
    if (!input_image_buffer) {
        std::cerr << "Error: Failed to load input image data!\n";
        return 1;
    }

    // Padding Logic
    int pad = 2;
    int padded_width = width + (2 * pad);
    int padded_height = height + (2 * pad);
    size_t padded_size = static_cast<size_t>(padded_width) * padded_height;

    std::vector<uint8_t> padded_input(padded_size, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            padded_input[(y + pad) * padded_width + (x + pad)] = input_image_buffer[y * width + x];
        }
    }

    std::vector<uint8_t> blurred(padded_size, 0);
    std::vector<int16_t> Gx(padded_size, 0);
    std::vector<int16_t> Gy(padded_size, 0);
    std::vector<uint8_t> magnitude(padded_size, 0);
    std::vector<uint8_t> direction(padded_size, 0);
    std::vector<uint8_t> nms(padded_size, 0);
    std::vector<uint8_t> final_edges(padded_size, 0);

    double total_gaussian = 0.0, total_sobel = 0.0, total_mag = 0.0, total_dir = 0.0, total_nms = 0.0, total_hysteresis = 0.0;
    
    const int NUM_ITERATIONS = 200; 
    std::cerr << "[*] Executing functions (" << NUM_ITERATIONS << " iterations) with padded dimensions...\n";

    uint64_t total_pipeline_cycles = 0;

    // Profiling Loop
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        
        uint64_t start_c = read_cycles();

        auto t0 = std::chrono::high_resolution_clock::now();
        gaussian_blur_2d_rvv(padded_input.data(), blurred.data(), padded_width, padded_height);
        
        auto t1 = std::chrono::high_resolution_clock::now();
        sobel_rvv(blurred.data(), Gx.data(), Gy.data(), padded_width, padded_height);
        
        auto t2 = std::chrono::high_resolution_clock::now();
        compute_magnitude_rvv(Gx.data(), Gy.data(), magnitude.data(), padded_width, padded_height);
        
        auto t3 = std::chrono::high_resolution_clock::now();
        compute_direction_rvv(Gx.data(), Gy.data(), direction.data(), padded_width, padded_height);
        
        auto t4 = std::chrono::high_resolution_clock::now();
        non_maximum_suppression_rvv(magnitude.data(), direction.data(), nms.data(), padded_width, padded_height);
        
        auto t5 = std::chrono::high_resolution_clock::now();
        apply_thresholding(nms.data(), final_edges.data(), padded_width, padded_height, low_thresh, high_thresh);
        auto t6 = std::chrono::high_resolution_clock::now();

        uint64_t end_c = read_cycles();
        total_pipeline_cycles += (end_c - start_c);

        total_gaussian += get_time_diff_ms(t0, t1);
        total_sobel += get_time_diff_ms(t1, t2);
        total_mag += get_time_diff_ms(t2, t3);
        total_dir += get_time_diff_ms(t3, t4);
        total_nms += get_time_diff_ms(t4, t5);
        total_hysteresis += get_time_diff_ms(t5, t6);
    }

    // Cropping and border clearing
    std::vector<uint8_t> unpadded_output(image_size, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unpadded_output[y * width + x] = final_edges[(y + pad) * padded_width + (x + pad)];
        }
    }

    int border_thickness = 2;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x < border_thickness || x >= width - border_thickness || 
                y < border_thickness || y >= height - border_thickness) {
                unpadded_output[y * width + x] = 0; 
            }
        }
    }

    save_raw_image("dummy_out.raw", unpadded_output.data(), width, height);

    // Process Profiling Data
    double avg_gaussian = total_gaussian / NUM_ITERATIONS;
    double avg_sobel = total_sobel / NUM_ITERATIONS;
    double avg_mag = total_mag / NUM_ITERATIONS;
    double avg_dir = total_dir / NUM_ITERATIONS;
    double avg_nms = total_nms / NUM_ITERATIONS;
    double avg_hysteresis = total_hysteresis / NUM_ITERATIONS;
    double total_avg_time = avg_gaussian + avg_sobel + avg_mag + avg_dir + avg_nms + avg_hysteresis;

    double avg_cycles = static_cast<double>(total_pipeline_cycles) / NUM_ITERATIONS;

    // Print Report
    std::cerr << "\n========================================================\n";
    std::cerr << "           HYBRID RVV PIPELINE PROFILING REPORT          \n";
    std::cerr << "========================================================\n";
    std::cerr << std::fixed << std::setprecision(3);
    std::cerr << "1. Gaussian Blur (RVV): " << std::setw(8) << avg_gaussian << " ms\n";
    std::cerr << "2. Sobel Operator(RVV): " << std::setw(8) << avg_sobel << " ms\n";
    std::cerr << "3. Magnitude     (RVV): " << std::setw(8) << avg_mag << " ms\n";
    std::cerr << "4. Direction     (RVV): " << std::setw(8) << avg_dir << " ms\n";
    std::cerr << "5. NMS (Scalar)       : " << std::setw(8) << avg_nms << " ms\n";
    std::cerr << "6. Hysteresis (Scalar): " << std::setw(8) << avg_hysteresis << " ms\n";
    std::cerr << "--------------------------------------------------------\n";
    std::cerr << "TOTAL TIME            : " << std::setw(8) << total_avg_time << " ms\n";
    std::cerr << "AVERAGE CYCLES        : " << static_cast<uint64_t>(avg_cycles) << " clock cycles\n";
    std::cerr << "========================================================\n";

    free(input_image_buffer);
    return 0;
}