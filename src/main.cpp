#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdlib>

#include "image_io.h"
#include "gaussian_blur.h"
#include "sobel.h"

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

    // Load real image using your custom 64-byte aligned function
    uint8_t* input_image_buffer = load_raw_image(input_filename, width, height);
    if (!input_image_buffer) {
        std::cerr << "Error: Failed to load input image: " << input_filename << std::endl;
        return 1;
    }
    std::cout << "Loaded input image: " << input_filename << " (" << width << "x" << height << ")" << std::endl;

    // Allocate buffers for pipeline stages
    std::vector<uint8_t> blurred(image_size);
    std::vector<int16_t> Gx(image_size), Gy(image_size);
    std::vector<uint8_t> magnitude(image_size);
    std::vector<float> angle(image_size);
    std::vector<uint8_t> nms(image_size);
    std::vector<uint8_t> final_edges(image_size);

    // Canny Edge Detection Pipeline Execution
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

    // Save final processed edges
    save_raw_image(output_filename, final_edges.data(), width, height);
    std::cout << "Final edges saved to: " << output_filename << std::endl;

    free(input_image_buffer);
    return 0;
}