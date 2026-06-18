#include <riscv_vector.h>
#include <cstdint>

extern const int16_t kernel5x5[5][5];

void gaussian_blur_2d_rvv(const uint8_t* input, uint8_t* output, int width, int height) {
    for (int y = 2; y < height - 2; ++y) {
        for (int x = 2; x < width - 2; ) {
            
            /* * (1) Operation: Set vector length based on remaining columns.
             * (2) LMUL: m1, as we process 8-bit pixels initially.
             * (3) VLEN: If VLEN is larger (e.g., 512), vl increases, reducing iterations.
             */
            size_t vl = __riscv_vsetvl_e8m1(width - 2 - x);

            /* * (1) Operation: Initialize 32-bit accumulator vector to zero.
             * (2) LMUL: m4, chosen to store 32-bit sums for 25 accumulations.
             * (3) VLEN: Automatically scales with hardware vector length.
             */
            vuint32m4_t vec_sum = __riscv_vmv_v_x_u32m4(0, vl);

            for (int ky = -2; ky <= 2; ++ky) {
                for (int kx = -2; kx <= 2; ++kx) {
                    int py = y + ky;
                    int px = x + kx;

                    /* * (1) Operation: Load vl pixels from memory.
                     * (2) LMUL: m1, matching the uint8_t input size.
                     * (3) VLEN: Loads as many pixels as the current VLEN allows.
                     */
                    vuint8m1_t vec_pixels = __riscv_vle8_v_u8m1(&input[py * width + px], vl);
                    
                    /* * (1) Operation: Widen 8-bit pixels to 16-bit to prevent overflow during multiply.
                     * (2) LMUL: m2, widening doubles the register group size.
                     * (3) VLEN: Consistent across different VLEN settings.
                     */
                    vuint16m2_t vec_pixels_16 = __riscv_vwaddu_vx_u16m2(vec_pixels, 0, vl);
                    
                    uint16_t k_val = (uint16_t)kernel5x5[ky + 2][kx + 2];
                    
                    /* * (1) Operation: Multiply 16-bit pixels by kernel and accumulate into 32-bit sum.
                     * (2) LMUL: m4, required to hold the 32-bit accumulation vector.
                     * (3) VLEN: Scales processing power; works correctly on any VLEN.
                     */
                    vec_sum = __riscv_vwmaccu_vx_u32m4(vec_sum, k_val, vec_pixels_16, vl);
                }
            }

            /* * (1) Operation: Fixed-point multiplication by 240 as approximation for /273.
             * (2) LMUL: m4, keeping data in 32-bit format.
             * (3) VLEN: Independent of VLEN size.
             */
            vuint32m4_t vec_mul = __riscv_vmul_vx_u32m4(vec_sum, 240, vl);

            /* * (1) Operation: Logical right shift by 16 bits to complete the division approximation.
             * (2) LMUL: m4, maintaining 32-bit data.
             * (3) VLEN: Performs shift in parallel for all vl elements.
             */
            vuint32m4_t vec_div = __riscv_vsrl_vx_u32m4(vec_mul, 16, vl);
            
            /* * (1) Operation: Narrowing 32-bit to 16-bit with saturation.
             * (2) LMUL: m2, reduces register group size by half.
             * (3) VLEN: Safely handles narrowing on any hardware vector length.
             */
           vuint16m2_t vec_narrow1 = __riscv_vnclipu_wx_u16m2(vec_div, 0, __RISCV_FRM_RNE, vl);

            /* * (1) Operation: Final narrowing to 8-bit uint, clamping to [0, 255].
             * (2) LMUL: m1, back to original input size.
             * (3) VLEN: Ensures saturation works for all elements regardless of vector length.
             */
            vuint8m1_t vec_out = __riscv_vnclipu_wx_u8m1(vec_narrow1, 0, __RISCV_FRM_RNE, vl);

            /* * (1) Operation: Store final result to output memory.
             * (2) LMUL: m1, aligns with output buffer type uint8_t.
             * (3) VLEN: Writes the entire vector chunk efficiently.
             */
            __riscv_vse8_v_u8m1(&output[y * width + x], vec_out, vl);

            x += vl;
        }
    }
}