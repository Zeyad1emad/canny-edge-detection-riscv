#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "../src/magnitude_rvv.h"

// Scalar reference using same Alpha-Max-Beta-Min formula (no normalization)
void compute_magnitude_scalar_ref(const int16_t* gx, const int16_t* gy,
                                   uint8_t* output, int width, int height) {
    int n = width * height;
    for (int i = 0; i < n; i++) {
        int ax = abs((int)gx[i]);
        int ay = abs((int)gy[i]);
        int mx = ax > ay ? ax : ay;
        int mn = ax < ay ? ax : ay;
        int mag = mx + ((mn * 13) >> 5);
        if (mag > 255) mag = 255;
        output[i] = (uint8_t)mag;
    }
}

void run_test(int width, int height, const char* label) {
    int n = width * height;

    int16_t* gx = new int16_t[n];
    int16_t* gy = new int16_t[n];
    uint8_t* out_scalar = new uint8_t[n];
    uint8_t* out_rvv    = new uint8_t[n];

    for (int i = 0; i < n; i++) {
        gx[i] = (int16_t)((i * 37 + 13) % 1021 - 510);
        gy[i] = (int16_t)((i * 53 + 7)  % 1021 - 510);
    }

    compute_magnitude_scalar_ref(gx, gy, out_scalar, width, height);
    compute_magnitude_rvv(gx, gy, out_rvv, width, height);

    int mismatches = 0;
    for (int i = 0; i < n; i++) {
        int diff = abs((int)out_scalar[i] - (int)out_rvv[i]);
        if (diff > 1) {
            printf("MISMATCH at [%d]: scalar=%d rvv=%d diff=%d\n",
                   i, out_scalar[i], out_rvv[i], diff);
            mismatches++;
        }
    }

    if (mismatches == 0)
        printf("[PASS] %s (%dx%d)\n", label, width, height);
    else
        printf("[FAIL] %s (%dx%d) — %d mismatches\n", label, width, height, mismatches);

    delete[] gx;
    delete[] gy;
    delete[] out_scalar;
    delete[] out_rvv;
}

int main() {
    printf("=== Magnitude RVV Equivalence Test ===\n");
    run_test(48,  48,  "non-power-of-two 48x48");
    run_test(100, 75,  "non-power-of-two 100x75");
    run_test(64,  64,  "power-of-two 64x64");
    run_test(1,   1,   "single pixel");
    printf("=== Done ===\n");
    return 0;
}
