#include "direction.h"
#include <cstdlib>

// Gradient direction quantization using integer cross-multiplication.
//
// Instead of computing atan2(Gy, Gx), we compare absolute values
// using scaled thresholds:
//   tan(22.5°) ≈ 2/5  →  ay/ax < 2/5  means angle < 22.5°
//   tan(67.5°) ≈ 12/5 →  ay/ax < 12/5 means angle < 67.5°
//
// Cross-multiply to avoid division:
//   ay/ax < 2/5  becomes  ay*5 < ax*2
//   ay/ax < 12/5 becomes  ay*5 < ax*12
//
// Sign of Gx/Gy is ignored — a gradient pointing left or right
// represents the same edge direction.

void compute_direction(const int16_t* Gx, const int16_t* Gy,
                       uint8_t* output, int width, int height) {

    for (int i = 0; i < width * height; i++) {

        int ax = abs((int)Gx[i]);
        int ay = abs((int)Gy[i]);

        uint8_t dir;

        if      (ay * 5 <  ax * 2)  dir = 0;
        else if (ay * 5 < ax * 12)  dir = 1;
        else if (ay * 5 > ax * 12)  dir = 2;
        else                        dir = 3;

        output[i] = dir;
    }
}
