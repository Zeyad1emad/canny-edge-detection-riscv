#pragma once
#include <cstdint>
#include <cmath>

// L1 norm: |Gx| + |Gy| (fast, integer only)
void compute_magnitude_l1(const int16_t* gx, const int16_t* gy,
                           uint8_t* output, int width, int height);

// L2 norm: sqrt(Gx^2 + Gy^2) (accurate, uses float)
void compute_magnitude_l2(const int16_t* gx, const int16_t* gy,
                           uint8_t* output, int width, int height);
