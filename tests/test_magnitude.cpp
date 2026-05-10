#include <gtest/gtest.h>
#include "../src/magnitude.h"
#include <cstring>

// Test 1: zero gradient produces zero output
TEST(MagnitudeL1, ZeroInput) {
    int16_t gx[16] = {0}, gy[16] = {0};
    uint8_t out[16];
    compute_magnitude_l1(gx, gy, out, 4, 4);
    for (int i = 0; i < 16; i++)
        EXPECT_EQ(out[i], 0);
}

TEST(MagnitudeL2, ZeroInput) {
    int16_t gx[16] = {0}, gy[16] = {0};
    uint8_t out[16];
    compute_magnitude_l2(gx, gy, out, 4, 4);
    for (int i = 0; i < 16; i++)
        EXPECT_EQ(out[i], 0);
}

// Test 2: maximum pixel should map to 255 after normalization
TEST(MagnitudeL1, MaxPixelIs255) {
    int16_t gx[4] = {100, 0, 50, 0};
    int16_t gy[4] = {0, 100, 50, 0};
    uint8_t out[4];
    compute_magnitude_l1(gx, gy, out, 4, 1);
    uint8_t max_out = 0;
    for (int i = 0; i < 4; i++)
        if (out[i] > max_out) max_out = out[i];
    EXPECT_EQ(max_out, 255);
}

TEST(MagnitudeL2, MaxPixelIs255) {
    int16_t gx[4] = {100, 0, 50, 0};
    int16_t gy[4] = {0, 100, 50, 0};
    uint8_t out[4];
    compute_magnitude_l2(gx, gy, out, 4, 1);
    uint8_t max_out = 0;
    for (int i = 0; i < 4; i++)
        if (out[i] > max_out) max_out = out[i];
    EXPECT_EQ(max_out, 255);
}

// Test 3: no output pixel exceeds 255
TEST(MagnitudeL1, NoOverflow) {
    int16_t gx[9] = {100,200,150,50,0,75,120,90,30};
    int16_t gy[9] = {30,50,100,200,150,75,20,110,60};
    uint8_t out[9];
    compute_magnitude_l1(gx, gy, out, 3, 3);
    for (int i = 0; i < 9; i++)
        EXPECT_LE(out[i], 255);
}

TEST(MagnitudeL2, NoOverflow) {
    int16_t gx[9] = {100,200,150,50,0,75,120,90,30};
    int16_t gy[9] = {30,50,100,200,150,75,20,110,60};
    uint8_t out[9];
    compute_magnitude_l2(gx, gy, out, 3, 3);
    for (int i = 0; i < 9; i++)
        EXPECT_LE(out[i], 255);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
