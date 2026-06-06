
#include "gaussian_blur.h"
const int16_t kernel5x5[5][5] = {
    {1, 4, 7, 4, 1},
    {4, 16, 26, 16, 4},
    {7, 26, 41, 26, 7},
    {4, 16, 26, 16, 4},
    {1, 4, 7, 4, 1}
};

const int16_t kernel1D[5] = {1, 4, 6, 4, 1};

