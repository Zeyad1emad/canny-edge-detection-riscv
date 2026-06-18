#ifndef SOBEL_RVV_H
#define SOBEL_RVV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =========================================================================
// Sobel function using RISC-V Vector (RVV) Intrinsics
// Outputs Gx and Gy are completely separated based on the required 
// Structure of Arrays (SoA) layout.
// =========================================================================
void sobel_rvv(const uint8_t* input, int16_t* Gx, int16_t* Gy, int width, int height);

#ifdef __cplusplus
}
#endif

#endif // SOBEL_RVV_H