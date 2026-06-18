#pragma once
#include <cstdint>

// RVV-optimized magnitude using Alpha-Max-Beta-Min approximation
// Formula: Mag ≈ max(|Gx|,|Gy|) + (min(|Gx|,|Gy|) * 13) >> 5
// This approximates sqrt(Gx^2 + Gy^2) without expensive sqrt or division
void compute_magnitude_rvv(const int16_t* gx, const int16_t* gy,
                            uint8_t* output, int width, int height);
