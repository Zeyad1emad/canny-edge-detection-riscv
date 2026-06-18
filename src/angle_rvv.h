#pragma once
#include <cstdint>

// RVV-accelerated version of compute_direction().
// Quantizes gradient angle into 4 directions using RVV vector masking
// and cross-multiplication instead of atan2.
//
// Output encoding:
//   0 →  0°  horizontal gradient (edge is vertical)
//   1 → 45°  diagonal
//   2 → 90°  vertical gradient (edge is horizontal)
//   3 → 135° other diagonal
//
// Gx and Gy are separate int16_t arrays (SoA layout).
// output must be pre-allocated: uint8_t* out = (uint8_t*)aligned_alloc(64, W*H);
void compute_direction_rvv(const int16_t* Gx, const int16_t* Gy,
                            uint8_t* output, int width, int height);
