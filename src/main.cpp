#include <cstdlib>
#include <cstdio>
#include "image_io.h"

int main() {
    const int W = 256;
    const int H = 256;
    const size_t size = W * H;

    // -------------------------------------------------------
    // 1. Generate and save all 3 test patterns
    // -------------------------------------------------------

    uint8_t* buf = static_cast<uint8_t*>(aligned_alloc(64, size));

    // Pattern 0 — White Rectangle
    test_image_generator(buf, W, H, 0);
    save_raw_image("rect.raw", buf, W, H);
    printf("Saved rect.raw\n");

    // Pattern 1 — Circle
    test_image_generator(buf, W, H, 1);
    save_raw_image("circle.raw", buf, W, H);
    printf("Saved circle.raw\n");

    // Pattern 2 — Diagonal Edge
    test_image_generator(buf, W, H, 2);
    save_raw_image("diagonal.raw", buf, W, H);
    printf("Saved diagonal.raw\n");

    free(buf);

    // -------------------------------------------------------
    // 2. Reload each file and verify it loads correctly
    // -------------------------------------------------------

    uint8_t* loaded = load_raw_image("rect.raw", W, H);
    if (loaded) {
        printf("Reloaded rect.raw OK — first pixel: %d, center pixel: %d\n",
               loaded[0], loaded[(H/2) * W + (W/2)]);
        free(loaded);
    }

    loaded = load_raw_image("circle.raw", W, H);
    if (loaded) {
        printf("Reloaded circle.raw OK — first pixel: %d, center pixel: %d\n",
               loaded[0], loaded[(H/2) * W + (W/2)]);
        free(loaded);
    }

    loaded = load_raw_image("diagonal.raw", W, H);
    if (loaded) {
        printf("Reloaded diagonal.raw OK — top-right pixel: %d, bottom-left pixel: %d\n",
               loaded[0 * W + (W-1)],       // top-right → should be 255 (white)
               loaded[(H-1) * W + 0]);       // bottom-left → should be 0 (black)
        free(loaded);
    }

    // -------------------------------------------------------
    // 3. Test error handling — load a file that doesn't exist
    // -------------------------------------------------------

    uint8_t* bad = load_raw_image("nonexistent.raw", W, H);
    if (!bad)
        printf("Error handling OK — got nullptr for missing file\n");

    return 0;
}