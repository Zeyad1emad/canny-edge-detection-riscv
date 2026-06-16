#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include "image_io.h"
#include "gaussian_blur.h"
#include "sobel.h"

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <width> <height> <pattern_type> [low_threshold] [high_threshold]" << std::endl;
        return 1;
    }

    int width = std::stoi(argv[1]);
    int height = std::stoi(argv[2]);
    int pattern_type = std::stoi(argv[3]);
    uint8_t low_thresh = (argc > 4) ? static_cast<uint8_t>(std::stoi(argv[4])) : 50;
    uint8_t high_thresh = (argc > 5) ? static_cast<uint8_t>(std::stoi(argv[5])) : 150;

    if (width <= 0 || height <= 0) {
        std::cerr << "Error: Width and height must be positive." << std::endl;
        return 1;
    }

    size_t image_size = static_cast<size_t>(width) * height;
    size_t aligned_size = ((image_size + 63) / 64) * 64;
    uint8_t* input_image_buffer = static_cast<uint8_t*>(aligned_alloc(64, aligned_size));
    if (!input_image_buffer) {
        std::cerr << "Error: Failed to allocate memory for input image." << std::endl;
        return 1;
    }

    test_image_generator(input_image_buffer, width, height, pattern_type);
    std::cout << "Generated input image with pattern type: " << pattern_type << std::endl;

    // Allocate buffers for intermediate and final results
    std::vector<uint8_t> blurred(image_size);
    std::vector<int16_t> Gx(image_size), Gy(image_size);
    std::vector<uint8_t> magnitude(image_size);
    std::vector<float> angle(image_size);
    std::vector<uint8_t> nms(image_size);
    std::vector<uint8_t> final_edges(image_size);

    // Canny Edge Detection Pipeline
    std::cout << "Step 1: Gaussian Blur (Separable)..." << std::endl;
    gaussian_blur_separable<uint8_t, int32_t, int16_t>(input_image_buffer, blurred.data(), width, height);

    std::cout << "Step 2: Sobel Operator..." << std::endl;
    compute_sobel(blurred.data(), Gx.data(), Gy.data(), width, height);
    
    std::cout << "Step 3: Magnitude and Angle..." << std::endl;
    compute_magnitude_angle(Gx.data(), Gy.data(), magnitude.data(), angle.data(), width, height);

    std::cout << "Step 4: Non-Maximum Suppression..." << std::endl;
    non_maximum_suppression(magnitude.data(), angle.data(), nms.data(), width, height);

    std::cout << "Step 5: Hysteresis Thresholding..." << std::endl;
    apply_thresholding(nms.data(), final_edges.data(), width, height, low_thresh, high_thresh);

    // Save the final edge result
    save_raw_image("output_edges.raw", final_edges.data(), width, height);
    std::cout << "Final edges saved to output_edges.raw" << std::endl;

    free(input_image_buffer);
    return 0;
}