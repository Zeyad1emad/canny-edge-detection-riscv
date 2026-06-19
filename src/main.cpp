#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <time.h>
#include <iomanip>
#include <fstream>
#include <x86intrin.h> // مخصصة لقراءة عداد الـ x86 Cycles

#include "image_io.h"
#include "gaussian_blur.h"
#include "sobel.h"

// Helper function to calculate time difference in milliseconds
double get_time_diff_ms(const struct timespec& start, const struct timespec& end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

int main(int argc, char** argv) {
    // =========================================================================
    // Print Binary File Size
    // =========================================================================
    std::ifstream exec_file(argv[0], std::ios::binary | std::ios::ate);
    if (exec_file.is_open()) {
        std::streamsize size = exec_file.tellg();
        std::cout << "[*] Binary Executable Size (" << argv[0] << "): " << size << " bytes\n";
        exec_file.close();
    } else {
        std::cerr << "[-] Warning: Could not read binary size.\n";
    }
    // =========================================================================

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
    std::cout << "\n[*] Starting Profiling Sweep (" << NUM_ITERATIONS << " iterations) for stable measurements...\n";

    // Profiling Loop
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        struct timespec t0, t1, t2, t3, t4, t5;

        // Stage 1: Gaussian Blur
        clock_gettime(CLOCK_MONOTONIC, &t0);
        gaussian_blur_separable<uint8_t, int32_t, int16_t>(input_image_buffer, blurred.data(), width, height);
        
        // Stage 2: Sobel Operator
        clock_gettime(CLOCK_MONOTONIC, &t1);
        compute_sobel(blurred.data(), Gx.data(), Gy.data(), width, height);
        
        // Stage 3: Magnitude and Angle
        clock_gettime(CLOCK_MONOTONIC, &t2);
        compute_magnitude_angle(Gx.data(), Gy.data(), magnitude.data(), angle.data(), width, height);

        // Stage 4: Non-Maximum Suppression
        clock_gettime(CLOCK_MONOTONIC, &t3);
        non_maximum_suppression(magnitude.data(), angle.data(), nms.data(), width, height);

        // Stage 5: Hysteresis Thresholding
        clock_gettime(CLOCK_MONOTONIC, &t4);
        apply_thresholding(nms.data(), final_edges.data(), width, height, low_thresh, high_thresh);
        clock_gettime(CLOCK_MONOTONIC, &t5);

        // Accumulate time for each stage
        total_gaussian += get_time_diff_ms(t0, t1);
        total_sobel += get_time_diff_ms(t1, t2);
        total_mag_angle += get_time_diff_ms(t2, t3);
        total_nms += get_time_diff_ms(t3, t4);
        total_hysteresis += get_time_diff_ms(t4, t5);
    }

    // Save final processed edges (from the last iteration)
    save_raw_image(output_filename, final_edges.data(), width, height);
    std::cout << "[*] Final edges saved to: " << output_filename << "\n" << std::endl;

    // Process Profiling Data
    double avg_gaussian = total_gaussian / NUM_ITERATIONS;
    double avg_sobel = total_sobel / NUM_ITERATIONS;
    double avg_mag_angle = total_mag_angle / NUM_ITERATIONS;
    double avg_nms = total_nms / NUM_ITERATIONS;
    double avg_hysteresis = total_hysteresis / NUM_ITERATIONS;
    double total_avg_time = avg_gaussian + avg_sobel + avg_mag_angle + avg_nms + avg_hysteresis;

    // Print Profiling Report
    std::cout << "========================================================\n";
    std::cout << "           PIPELINE PROFILING REPORT (PER FRAME)        \n";
    std::cout << "========================================================\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "1. Gaussian Blur : " << std::setw(8) << avg_gaussian << " ms  |  " 
              << std::setw(5) << (avg_gaussian / total_avg_time) * 100.0 << " %\n";
    std::cout << "2. Sobel Operator: " << std::setw(8) << avg_sobel << " ms  |  " 
              << std::setw(5) << (avg_sobel / total_avg_time) * 100.0 << " %\n";
    std::cout << "3. Mag & Angle   : " << std::setw(8) << avg_mag_angle << " ms  |  " 
              << std::setw(5) << (avg_mag_angle / total_avg_time) * 100.0 << " %\n";
    std::cout << "4. NMS           : " << std::setw(8) << avg_nms << " ms  |  " 
              << std::setw(5) << (avg_nms / total_avg_time) * 100.0 << " %\n";
    std::cout << "5. Hysteresis    : " << std::setw(8) << avg_hysteresis << " ms  |  " 
              << std::setw(5) << (avg_hysteresis / total_avg_time) * 100.0 << " %\n";
    std::cout << "--------------------------------------------------------\n";
    std::cout << "TOTAL TIME       : " << std::setw(8) << total_avg_time << " ms  |  100.0 %\n";
    std::cout << "========================================================\n";

    free(input_image_buffer);
    return 0;
}