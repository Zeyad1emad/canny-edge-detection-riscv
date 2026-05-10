#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>

#include "../src/direction.h"

// ─── Test 1 — vertical edge → direction 0 ───────────────────────────────────
// Large Gx, zero Gy → angle is 0° → direction must be 0

TEST(DirectionTest, VerticalEdgeGivesDirection0) {
    const int W = 5, H = 1;
    int16_t Gx[] = {1000, 1000, 1000, 1000, 1000};
    int16_t Gy[] = {0,    0,    0,    0,    0};
    uint8_t dir[5];

    compute_direction(Gx, Gy, dir, W, H);

    for (int i = 0; i < 5; i++)
        EXPECT_EQ(dir[i], 0) << "Failed at index " << i;
}

// ─── Test 2 — horizontal edge → direction 2 ─────────────────────────────────
// Zero Gx, large Gy → angle is 90° → direction must be 2

TEST(DirectionTest, HorizontalEdgeGivesDirection2) {
    const int W = 5, H = 1;
    int16_t Gx[] = {0,    0,    0,    0,    0};
    int16_t Gy[] = {1000, 1000, 1000, 1000, 1000};
    uint8_t dir[5];

    compute_direction(Gx, Gy, dir, W, H);

    for (int i = 0; i < 5; i++)
        EXPECT_EQ(dir[i], 2) << "Failed at index " << i;
}

// ─── Test 3 — diagonal edge → direction 1 or 3 ──────────────────────────────
// Equal Gx and Gy → angle is 45° → direction must be 1 or 3

TEST(DirectionTest, DiagonalEdgeGivesDirection1or3) {
    const int W = 5, H = 1;
    int16_t Gx[] = {1000, 1000, 1000, 1000, 1000};
    int16_t Gy[] = {1000, 1000, 1000, 1000, 1000};
    uint8_t dir[5];

    compute_direction(Gx, Gy, dir, W, H);

    for (int i = 0; i < 5; i++)
        EXPECT_TRUE(dir[i] == 1 || dir[i] == 3)
            << "Expected 1 or 3 at index " << i << " but got " << (int)dir[i];
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
