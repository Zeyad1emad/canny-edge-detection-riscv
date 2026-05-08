#ifndef CANNY_H
#define CANNY_H

#include <vector>
#include <cstdint>

// Function for Phase 2: Gaussian Blur (5x5)
void gaussian_blur(const uint8_t* input, uint8_t* output, int width, int height);

// Function for Phase 2: Sobel Operator
void sobel_operator(const uint8_t* input, uint8_t* magnitude, uint8_t* direction, int width, int height);

#endif
