#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "../src/direction.h"
#include "../src/angle_rvv.h"

// Test 1: Vertical edge (large Gx, zero Gy) → direction 0
TEST(AngleRVVTest, VerticalEdgeGivesDirection0) {
    const int N = 16;
    int16_t Gx[N], Gy[N];
    uint8_t scalar_out[N], rvv_out[N];
    for (int i = 0; i < N; i++) { Gx[i] = 1000; Gy[i] = 0; }

    compute_direction(Gx, Gy, scalar_out, N, 1);
    compute_direction_rvv(Gx, Gy, rvv_out, N, 1);

    for (int i = 0; i < N; i++)
        EXPECT_EQ(rvv_out[i], scalar_out[i]) << "Mismatch at index " << i;
}

// Test 2: Horizontal edge (zero Gx, large Gy) → direction 2
TEST(AngleRVVTest, HorizontalEdgeGivesDirection2) {
    const int N = 16;
    int16_t Gx[N], Gy[N];
    uint8_t scalar_out[N], rvv_out[N];
    for (int i = 0; i < N; i++) { Gx[i] = 0; Gy[i] = 1000; }

    compute_direction(Gx, Gy, scalar_out, N, 1);
    compute_direction_rvv(Gx, Gy, rvv_out, N, 1);

    for (int i = 0; i < N; i++)
        EXPECT_EQ(rvv_out[i], scalar_out[i]) << "Mismatch at index " << i;
}

// Test 3: Diagonal edge (equal Gx and Gy) → direction 1
TEST(AngleRVVTest, DiagonalEdgeGivesDirection1) {
    const int N = 16;
    int16_t Gx[N], Gy[N];
    uint8_t scalar_out[N], rvv_out[N];
    for (int i = 0; i < N; i++) { Gx[i] = 1000; Gy[i] = 1000; }

    compute_direction(Gx, Gy, scalar_out, N, 1);
    compute_direction_rvv(Gx, Gy, rvv_out, N, 1);

    for (int i = 0; i < N; i++)
        EXPECT_EQ(rvv_out[i], scalar_out[i]) << "Mismatch at index " << i;
}

// Test 4: Equivalence on random data — scalar vs RVV must match exactly
TEST(AngleRVVTest, RandomDataEquivalence) {
    const int W = 64, H = 64;
    std::vector<int16_t> Gx(W*H), Gy(W*H);
    std::vector<uint8_t> scalar_out(W*H), rvv_out(W*H);

    srand(42);
    for (int i = 0; i < W*H; i++) {
        Gx[i] = (int16_t)(rand() % 2000 - 1000);
        Gy[i] = (int16_t)(rand() % 2000 - 1000);
    }

    compute_direction(Gx.data(), Gy.data(), scalar_out.data(), W, H);
    compute_direction_rvv(Gx.data(), Gy.data(), rvv_out.data(), W, H);

    for (int i = 0; i < W*H; i++)
        EXPECT_EQ(rvv_out[i], scalar_out[i]) << "Mismatch at pixel " << i;
}

// Test 5: Non-multiple-of-vl size (tests strip-mining tail handling)
TEST(AngleRVVTest, NonMultipleOfVLSize) {
    const int N = 37; // odd size to test tail
    int16_t Gx[N], Gy[N];
    uint8_t scalar_out[N], rvv_out[N];

    srand(7);
    for (int i = 0; i < N; i++) {
        Gx[i] = (int16_t)(rand() % 2000 - 1000);
        Gy[i] = (int16_t)(rand() % 2000 - 1000);
    }

    compute_direction(Gx, Gy, scalar_out, N, 1);
    compute_direction_rvv(Gx, Gy, rvv_out, N, 1);

    for (int i = 0; i < N; i++)
        EXPECT_EQ(rvv_out[i], scalar_out[i]) << "Mismatch at index " << i;
}
