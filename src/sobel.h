#ifndef SOBEL_H
#define SOBEL_H

#include <cstdint>

// =========================================================================
// 1. Template for Core Sobel Gradient Computation (Border-Safe Version)
// =========================================================================
/**
 * @brief Generic template to compute Sobel-X and Sobel-Y gradients.
 * @details Initializes the entire output with zeros, then processes only the 
 * internal pixels to leave a strictly clean 0-pixel border.
 * @tparam PixelT  Data type of input pixels (typically uint8_t for grayscale).
 * @tparam AccumT  Data type for internal accumulations (int32_t to prevent overflow).
 * @tparam KernelT Data type for the output gradients (int16_t to support signed values).
 */
template <typename PixelT, typename AccumT, typename KernelT>
void sobel_core_template(const PixelT* input, KernelT* Gx, KernelT* Gy, int width, int height) {
    
    // Step 1: Strictly clear the entire output arrays to 0 (Handles borders perfectly)
    for (int i = 0; i < width * height; ++i) {
        Gx[i] = 0;
        Gy[i] = 0;
    }

    // Sobel-X Kernel: Detects vertical changes/edges
    const KernelT kernel_x[3][3] = {{-1,  0,  1}, 
                                    {-2,  0,  2}, 
                                    {-1,  0,  1}};

    // Sobel-Y Kernel: Detects horizontal changes/edges
    const KernelT kernel_y[3][3] = {{-1, -2, -1}, 
                                    { 0,  0,  0}, 
                                    { 1,  2,  1}};

    // Step 2: Loop ONLY through internal pixels (Safe boundaries: from 1 to height-1 / width-1)
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            AccumT sum_x = 0;
            AccumT sum_y = 0;

            // Compute convolution using the 3x3 neighborhood (No boundary IF checks needed)
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    PixelT val = input[(y + ky) * width + (x + kx)];
                    sum_x += (AccumT)val * kernel_x[ky + 1][kx + 1];
                    sum_y += (AccumT)val * kernel_y[ky + 1][kx + 1];
                }
            }
            
            // Store final accumulated gradients in Structure of Arrays (SoA)
            Gx[y * width + x] = (KernelT)sum_x;
            Gy[y * width + x] = (KernelT)sum_y;
        }
    }
}

void compute_sobel(const uint8_t* input, int16_t* Gx, int16_t* Gy, int width, int height);
void compute_magnitude_angle(const int16_t* Gx, const int16_t* Gy, uint8_t* magnitude, float* angle, int width, int height);
// 1. Version for Host (Takes Exact Float Angle)
void non_maximum_suppression(const uint8_t* magnitude, const float* angle, uint8_t* nms_output, int width, int height);

// 2. Version for RVV (Takes Quantized uint8_t Direction)
void non_maximum_suppression(const uint8_t* magnitude, const uint8_t* direction, uint8_t* nms_output, int width, int height);
void apply_thresholding(const uint8_t* nms_output, uint8_t* final_edges, int width, int height, uint8_t low_thresh, uint8_t high_thresh);

#endif 