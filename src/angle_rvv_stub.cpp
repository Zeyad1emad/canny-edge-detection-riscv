// Host stub: RVV intrinsics are not available on x86.
// For host testing, the RVV function falls back to scalar compute_direction.
#include "angle_rvv.h"
#include "direction.h"

void compute_direction_rvv(const int16_t* Gx, const int16_t* Gy,
                            uint8_t* output, int width, int height) {
    compute_direction(Gx, Gy, output, width, height);
}
