#include "magnitude.h"
#include <cstdlib>
#include <cmath>

void compute_magnitude_l1(const int16_t* gx, const int16_t* gy,
                           uint8_t* output, int width, int height) {
    int n = width * height;

    // Pass 1: compute raw magnitudes and find max
    int32_t* temp = new int32_t[n];
    int32_t max_val = 0;

    for (int i = 0; i < n; i++) {
        int32_t mag = abs((int32_t)gx[i]) + abs((int32_t)gy[i]);
        temp[i] = mag;
        if (mag > max_val) max_val = mag;
    }

    // Pass 2: normalize to [0, 255]
    for (int i = 0; i < n; i++) {
        if (max_val == 0)
            output[i] = 0;
        else
            output[i] = (uint8_t)((temp[i] * 255) / max_val);
    }

    delete[] temp;
}

void compute_magnitude_l2(const int16_t* gx, const int16_t* gy,
                           uint8_t* output, int width, int height) {
    int n = width * height;

    // Pass 1: compute raw magnitudes and find max
    float* temp = new float[n];
    float max_val = 0.0f;

    for (int i = 0; i < n; i++) {
        float mag = sqrtf((float)gx[i] * gx[i] + (float)gy[i] * gy[i]);
        temp[i] = mag;
        if (mag > max_val) max_val = mag;
    }

    // Pass 2: normalize to [0, 255]
    for (int i = 0; i < n; i++) {
        if (max_val == 0.0f)
            output[i] = 0;
        else
            output[i] = (uint8_t)((temp[i] / max_val) * 255.0f);
    }

    delete[] temp;
}
