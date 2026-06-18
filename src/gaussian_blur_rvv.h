#ifndef GAUSSIAN_BLUR_RVV_H
#define GAUSSIAN_BLUR_RVV_H

#include <stdint.h>

void gaussian_blur_2d_rvv(const uint8_t* input, uint8_t* output, int width, int height);

#endif // GAUSSIAN_BLUR_RVV_H