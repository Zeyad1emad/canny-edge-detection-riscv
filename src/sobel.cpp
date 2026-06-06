#include "sobel.h"
#include <vector>
#include <cmath> // Required for std::sqrt and std::atan2

// =========================================================================
// 1. Core Task Function
// =========================================================================
void compute_sobel(const uint8_t* input, int16_t* Gx, int16_t* Gy, int width, int height) {
    // Instantiates the core template using safe data types to avoid data overflow
    sobel_core_template<uint8_t, int32_t, int16_t>(input, Gx, Gy, width, height);
}

// =========================================================================
// 2. Magnitude and Angle Calculation
// =========================================================================
void compute_magnitude_angle(const int16_t* Gx, const int16_t* Gy, uint8_t* magnitude, float* angle, int width, int height) {
    for (int i = 0; i < width * height; ++i) {
        // Calculate edge strength: magnitude = sqrt(Gx^2 + Gy^2)
        int32_t mag = std::sqrt(Gx[i] * Gx[i] + Gy[i] * Gy[i]);
        
        // Clamp the value to safely fit inside an 8-bit unsigned integer [0-255]
        magnitude[i] = (mag > 255) ? 255 : (uint8_t)mag;

        // Calculate edge direction in radians, then convert to degrees [0 to 180]
        angle[i] = std::atan2(Gy[i], Gx[i]) * (180.0f / 3.14159265f);
        if (angle[i] < 0) {
            angle[i] += 180.0f; // Map negative angles to the positive 0-180 range
        }
    }
}

// =========================================================================
// 3. Non-Maximum Suppression (NMS) - Edge Thinning
// =========================================================================
void non_maximum_suppression(const uint8_t* magnitude, const float* angle, uint8_t* nms_output, int width, int height) {
    // Initialize the output array with zeros
    for (int i = 0; i < width * height; ++i) { nms_output[i] = 0; }

    // Loop through internal pixels, ignoring the outermost 1-pixel border
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int idx = y * width + x;
            float a = angle[idx];
            
            uint8_t neighbor_1 = 0;
            uint8_t neighbor_2 = 0;

            // Determine the two neighbors along the gradient direction line
            // Case A: Horizontal Edge Direction (Approx. 0 or 180 degrees)
            if ((a >= 0 && a < 22.5) || (a >= 157.5 && a <= 180)) {
                neighbor_1 = magnitude[y * width + (x + 1)]; // Right neighbor
                neighbor_2 = magnitude[y * width + (x - 1)]; // Left neighbor
            }
            // Case B: Diagonal 45 Edge Direction
            else if (a >= 22.5 && a < 67.5) {
                neighbor_1 = magnitude[(y + 1) * width + (x - 1)]; // Bottom-Left neighbor
                neighbor_2 = magnitude[(y - 1) * width + (x + 1)]; // Top-Right neighbor
            }
            // Case C: Vertical Edge Direction (Approx. 90 degrees)
            else if (a >= 67.5 && a < 112.5) {
                neighbor_1 = magnitude[(y + 1) * width + x]; // Bottom neighbor
                neighbor_2 = magnitude[(y - 1) * width + x]; // Top neighbor
            }
            // Case D: Diagonal 135 Edge Direction
            else if (a >= 112.5 && a < 157.5) {
                neighbor_1 = magnitude[(y - 1) * width + (x - 1)]; // Top-Left neighbor
                neighbor_2 = magnitude[(y + 1) * width + (x + 1)]; // Bottom-Right neighbor
            }

            // Suppress pixel (set to 0) if it's not the local maximum compared to its neighbors
            if (magnitude[idx] >= neighbor_1 && magnitude[idx] >= neighbor_2) {
                nms_output[idx] = magnitude[idx]; // Keep it, it is a local peak
            } else {
                nms_output[idx] = 0; // Suppress it
            }
        }
    }
}

// =========================================================================
// 4.Double Thresholding and Hysteresis 
// =========================================================================
void apply_thresholding(const uint8_t* nms_output, uint8_t* final_edges, int width, int height, uint8_t low_thresh, uint8_t high_thresh) {
    
    // Create a strict snapshot buffer for initial classification
    std::vector<uint8_t> temp_classification(width * height, 0);

    // Step 1: Initial classification (Read from NMS, write to Temp Buffer)
    for (int i = 0; i < width * height; ++i) {
        if (nms_output[i] >= high_thresh) {
            temp_classification[i] = 255; // Strong
        } else if (nms_output[i] >= low_thresh) {
            temp_classification[i] = 50;  // Weak
        } else {
            temp_classification[i] = 0;   // Non-edge
        }
    }

    // Step 2: Hysteresis Decision (Read strictly from Temp, write to Final)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            
            if (temp_classification[idx] == 255) {
                final_edges[idx] = 255; // Strong edges always survive
            } 
            else if (temp_classification[idx] == 50) {
                bool connected_to_strong = false;
                
                // Scan 8-connected neighbors ONLY from the initial unmodified snapshot
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int py = y + ky;
                        int px = x + kx;
                        
                        // Strict boundary check
                        if (py >= 0 && py < height && px >= 0 && px < width) {
                            // Read ONLY from the snapshot where pixels haven't cascaded yet
                            if (temp_classification[py * width + px] == 255) {
                                connected_to_strong = true;
                                break; // Found a valid connection, no need to continue scanning neighbors
                            }
                        }
                    }
                    if (connected_to_strong) break;
                }
                
                // If it was connected to an ORIGINAL strong edge, keep it. Otherwise, kill it.
                final_edges[idx] = connected_to_strong ? 255 : 0;
            } 
            else {
                final_edges[idx] = 0; // Non-edges stay 0
            }
        }
    }
}