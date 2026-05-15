#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "image_io.h"
// ---------------------------------------------------------------------------
// Function 1 — load_raw_image
// ---------------------------------------------------------------------------
uint8_t* load_raw_image(const char* filename, int width, int height) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "load_raw_image: cannot open '%s'\n", filename);
        return nullptr;
    }

    size_t size = (size_t)width * height;

    // 64-byte alignment required for RVV vector loads in later phases
    uint8_t* buf = static_cast<uint8_t*>(aligned_alloc(64, size));
    if (!buf) {
        fprintf(stderr, "load_raw_image: aligned_alloc failed\n");
        fclose(f);
        return nullptr;
    }

    size_t read = fread(buf, 1, size, f);
    fclose(f);

    if (read != size) {
        fprintf(stderr, "load_raw_image: expected %zu bytes, got %zu\n", size, read);
        free(buf);
        return nullptr;
    }

    return buf;  // caller must free()
}

// ---------------------------------------------------------------------------
// Function 2 — save_raw_image
// ---------------------------------------------------------------------------
void save_raw_image(const char* filename, const uint8_t* img, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "save_raw_image: cannot open '%s' for writing\n", filename);
        return;
    }

    size_t size = (size_t)width * height;
    size_t written = fwrite(img, 1, size, f);
    if (written != size) {
        fprintf(stderr, "save_raw_image: wrote %zu of %zu bytes\n", written, size);
    }

    fclose(f);
}

// ---------------------------------------------------------------------------
// Function 3 — test_image_generator
// ---------------------------------------------------------------------------
void test_image_generator(uint8_t* output, int width, int height, int pattern_type) {
    switch (static_cast<Pattern>(pattern_type)) {

    // --- WHITE_RECT: black background, white filled rectangle in the center ---
    case Pattern::WHITE_RECT: {
        memset(output, 0, (size_t)width * height);

        int rx0 = width  / 4;
        int ry0 = height / 4;
        int rx1 = width  * 3 / 4;
        int ry1 = height * 3 / 4;

        for (int r = ry0; r < ry1; ++r)
            for (int c = rx0; c < rx1; ++c)
                output[r * width + c] = 255;
        break;
    }

    // --- CIRCLE: black background, white filled circle in the center ---
    case Pattern::CIRCLE: {
        memset(output, 0, (size_t)width * height);

        float cx = width  / 2.0f;
        float cy = height / 2.0f;
        float radius = (float)(((width < height) ? width : height)) / 4.0f;

        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                float dx = c - cx;
                float dy = r - cy;
                if (dx * dx + dy * dy <= radius * radius)
                    output[r * width + c] = 255;
                else
                    output[r * width + c] = 0;
            }
        }
        break;
    }

    // --- DIAGONAL_EDGE: black lower-left triangle, white upper-right triangle ---
    // Out-of-bounds boundary assumption: zero-padding (same as Gaussian/Sobel).
    // The diagonal runs from (0, height-1) to (width-1, 0).
    // Pixels above the diagonal (upper-right) = 255; below (lower-left) = 0.
    case Pattern::DIAGONAL_EDGE: {
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                // Normalised position along diagonal:
                // white if  c/width + r/height < 1  →  c*height + r*width < width*height
                if (c * height + r * width < width * height)
                    output[r * width + c] = 255;
                else
                    output[r * width + c] = 0;
            }
        }
        break;
    }

    default:
        fprintf(stderr, "test_image_generator: unknown pattern_type %d\n", pattern_type);
        memset(output, 0, (size_t)width * height);
        break;
    }
}