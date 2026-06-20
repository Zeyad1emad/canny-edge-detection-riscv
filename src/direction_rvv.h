#ifndef DIRECTION_RVV_H
#define DIRECTION_RVV_H

#include <cstdint>

void compute_direction_rvv(const int16_t* Gx, const int16_t* Gy, uint8_t* output, int width, int height);

#endif // DIRECTION_RVV_H