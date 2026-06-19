#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <iomanip>

// Core Pipeline Headers
#include "image_io.h"
#include "sobel.h"

// RVV Optimized Headers
#include "gaussian_blur_rvv.h"
#include "sobel_rvv.h"
#include "magnitude_rvv.h"
#include "direction_rvv.h"

inline uint64_t read_cycles() {
    uint64_t cycles;
#ifdef __riscv
    asm volatile("rdcycle %0" : "=r"(cycles));
#else
    cycles = 0; 
#endif
    return cycles;
}

double get_time_diff_ms(uint64_t start, uint64_t end) {
    return static_cast<double>(end - start) / 100000.0; 
}

int main(int argc, char** argv) {
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
    
    uint8_t* input_image_buffer = load_raw_image("dummy_in.raw", width, height);
    if (!input_image_buffer) {
        std::cerr << "Error: Failed to load input image data!\n";
        return 1;
    }

    // ---------------------------------------------------------
    // 1. Padding Logic Setup
    // Adding 2 pixels padding on all sides (top, bottom, left, right)
    // ---------------------------------------------------------
    int pad = 2;
    int padded_width = width + (2 * pad);
    int padded_height = height + (2 * pad);
    size_t padded_size = static_cast<size_t>(padded_width) * padded_height;

    // Initialize padded input buffer with zeros (Zero-Padding)
    std::vector<uint8_t> padded_input(padded_size, 0);

    // Copy original image into the center of the padded buffer
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            padded_input[(y + pad) * padded_width + (x + pad)] = input_image_buffer[y * width + x];
        }
    }

    // Allocate intermediate buffers using the PADDED size
    std::vector<uint8_t> blurred(padded_size, 0);
    std::vector<int16_t> Gx(padded_size, 0);
    std::vector<int16_t> Gy(padded_size, 0);
    std::vector<uint8_t> magnitude(padded_size, 0);
    std::vector<uint8_t> direction(padded_size, 0);
    std::vector<uint8_t> nms(padded_size, 0);
    std::vector<uint8_t> final_edges(padded_size, 0);

    double total_gaussian = 0.0, total_sobel = 0.0, total_mag = 0.0, total_dir = 0.0, total_nms = 0.0, total_hysteresis = 0.0;
    
    const int NUM_ITERATIONS = 1; 
    std::cerr << "[*] Executing functions (" << NUM_ITERATIONS << " iterations) with padded dimensions (" << padded_width << "x" << padded_height << ")...\n";

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        uint64_t t0 = read_cycles();
        
        // Pass padded dimensions to all RVV functions
        gaussian_blur_2d_rvv(padded_input.data(), blurred.data(), padded_width, padded_height);
        
        uint64_t t1 = read_cycles();
        sobel_rvv(blurred.data(), Gx.data(), Gy.data(), padded_width, padded_height);
        
        uint64_t t2 = read_cycles();
        compute_magnitude_rvv(Gx.data(), Gy.data(), magnitude.data(), padded_width, padded_height);
        
        uint64_t t3 = read_cycles();
        compute_direction_rvv(Gx.data(), Gy.data(), direction.data(), padded_width, padded_height);
        
        uint64_t t4 = read_cycles();
        non_maximum_suppression(magnitude.data(), direction.data(), nms.data(), padded_width, padded_height);
        
        uint64_t t5 = read_cycles();
        apply_thresholding(nms.data(), final_edges.data(), padded_width, padded_height, low_thresh, high_thresh);
        uint64_t t6 = read_cycles();

        total_gaussian += get_time_diff_ms(t0, t1);
        total_sobel += get_time_diff_ms(t1, t2);
        total_mag += get_time_diff_ms(t2, t3);
        total_dir += get_time_diff_ms(t3, t4);
        total_nms += get_time_diff_ms(t4, t5);
        total_hysteresis += get_time_diff_ms(t5, t6);
    }

    // ---------------------------------------------------------
    // 2. Cropping Logic Setup
    // Extract the valid original size image from the padded result
    // ---------------------------------------------------------
    std::vector<uint8_t> unpadded_output(image_size, 0);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unpadded_output[y * width + x] = final_edges[(y + pad) * padded_width + (x + pad)];
        }
    }

    // ---------------------------------------------------------
    // [NEW/MODIFIED] 3. Fast Border Clearing (Remove False Edges)
    // ---------------------------------------------------------
    int border_thickness = 2;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x < border_thickness || x >= width - border_thickness || 
                y < border_thickness || y >= height - border_thickness) {
                unpadded_output[y * width + x] = 0; // صب اللون الأسود في البرواز الخارجي
            }
        }
    }

    std::cerr << "[*] Processing complete. Saving unpadded edges to STDOUT...\n";
    // Save the original size unpadded output (now clean from fake borders)
    save_raw_image("dummy_out.raw", unpadded_output.data(), width, height);

    double avg_gaussian = total_gaussian / NUM_ITERATIONS;
    double avg_sobel = total_sobel / NUM_ITERATIONS;
    double avg_mag = total_mag / NUM_ITERATIONS;
    double avg_dir = total_dir / NUM_ITERATIONS;
    double avg_nms = total_nms / NUM_ITERATIONS;
    double avg_hysteresis = total_hysteresis / NUM_ITERATIONS;
    double total_avg_time = avg_gaussian + avg_sobel + avg_mag + avg_dir + avg_nms + avg_hysteresis;

    std::cerr << "\n========================================================\n";
    std::cerr << "          HYBRID RVV PIPELINE PROFILING REPORT          \n";
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
    std::cerr << "========================================================\n";

    free(input_image_buffer);
    return 0;
}