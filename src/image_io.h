#pragma once
#include <cstdint>
#include <cstdlib>

enum class Pattern {
    WHITE_RECT    = 0,
    CIRCLE        = 1,
    DIAGONAL_EDGE = 2
};

// Reads a raw grayscale file from disk.
// Returns aligned buffer (caller must free()), or nullptr on error.
uint8_t* load_raw_image(const char* filename, int width, int height);

// Writes a raw grayscale buffer to disk.
void save_raw_image(const char* filename, const uint8_t* img, int width, int height);

// Fills 'output' (already allocated, width*height bytes) with a test pattern.
// pattern_type: 0 = WHITE_RECT, 1 = CIRCLE, 2 = DIAGONAL_EDGE
void test_image_generator(uint8_t* output, int width, int height, int pattern_type);