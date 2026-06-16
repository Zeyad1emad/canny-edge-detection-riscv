#pragma once
#include <cstdint>

// Quantizes gradient angle into 4 directions using integer arithmetic only.
// No atan2, no floating point.
//
// Output encoding:
//   0 →  0°  horizontal gradient (edge is vertical)
//   1 → 45°  diagonal
//   2 → 90°  vertical gradient (edge is horizontal)
//   3 → 135° other diagonal
//
// Gx and Gy are separate int16_t arrays (SoA layout) from compute_sobel().
// output must be pre-allocated by caller: uint8_t* out = (uint8_t*)aligned_alloc(64, W*H);

void compute_direction(const int16_t* Gx, const int16_t* Gy,
                       uint8_t* output, int width, int height);
