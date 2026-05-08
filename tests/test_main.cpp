#include <gtest/gtest.h>
#include "../src/canny.h"

// Basic Test: Checking if the setup works
TEST(GlobalSetup, EnvironmentTest) {
    ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
