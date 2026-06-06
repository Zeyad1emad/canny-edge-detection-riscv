#ifndef CANNY_PIPELINE_H
#define CANNY_PIPELINE_H

#include <cstdint>
#include <cstdlib>

/**
 * Task 1: Image I/O Functions
 * Responsibility: Loading from disk and saving results in raw format.
 */

// Loads a raw grayscale image. Uses aligned_alloc(64) for RVV compatibility.
uint8_t* load_raw_image(const char* filename, int width, int height);

// Saves a raw grayscale image to disk.
void save_raw_image(const char* filename, const uint8_t* img, int width, int height);

// Generates known patterns (Rectangle, Circle, Lines) for internal testing.
void test_image_generator(uint8_t* output, int width, int height, int pattern_type);


/**
 * Task 2: Gaussian Blur (Smoothing)
 * Responsibility: Removing noise using a 5x5 Gaussian kernel.
 */

// Applies 5x5 convolution. Accumulates in int32_t to avoid overflow. 
// Uses zero-padding for boundary handling.
/*
template <typename PixelT, typename AccumT, typename KernelT>
void gaussian_blur_2d(const PixelT* input, PixelT* output, int width, int height);

template <typename PixelT, typename AccumT, typename KernelT>
void gaussian_blur_separable(const PixelT* input, PixelT* output, int width, int height);
*/


/*
 * Task 3: Sobel Gradient Computation
 * Responsibility: Detecting edges using 3x3 Sobel-X and Sobel-Y kernels.
 */

// Computes horizontal (Gx) and vertical (Gy) gradients.
// Stores outputs in separate int16_t arrays (Structure of Arrays - SoA).
void compute_sobel(const uint8_t* input, int16_t* Gx, int16_t* Gy, int width, int height);


/**
 * Task 4: Gradient Magnitude
 * Responsibility: Combining Gx and Gy to find edge strength.
 */

// Computes Magnitude = sqrt(Gx^2 + Gy^2) or |Gx| + |Gy|.
// Normalizes the final result to [0, 255].
void compute_magnitude(const int16_t* Gx, const int16_t* Gy, uint8_t* output, int width, int height, bool use_L2);


/**
 * Task 5: Gradient Direction (Quantization)
 * Responsibility: Categorizing edge angles into (0, 45, 90, 135 degrees).
 */

// Quantizes direction using cross-multiplication instead of atan2().
void compute_direction(const int16_t* Gx, const int16_t* Gy, uint8_t* output, int width, int height);

#endif // CANNY_PIPELINE_H