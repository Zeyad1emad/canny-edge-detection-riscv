#include <riscv_vector.h>
#include <stdint.h>
#include <cstring> // Required for memset
#include "sobel_rvv.h"

/*
 * ============================================================================
 * DELIVERABLE: Why int16_t is sufficient for Sobel but not Gaussian?
 * ============================================================================
 * For Sobel: The maximum possible gradient value occurs when half the kernel 
 * multiplies by 0 and the other half multiplies 255 by the maximum weights.
 * Max |Gx| or |Gy| = (1*255) + (2*255) + (1*255) = 4 * 255 = 1020.
 * Since 1020 easily fits within a signed 16-bit integer (max 32767), int16_t 
 * is perfectly sufficient and prevents overflow.
 * For Gaussian: The kernel often has positive weights that sum to a larger 
 * value, and intermediate sums of multiplying 255 by these weights can easily 
 * exceed the 16-bit limit before the final division, requiring int32_t.
 * ============================================================================
 */

// =========================================================================
// RVV Accelerated Sobel Filter Implementation (MAX OPTIMIZED)
// =========================================================================
void sobel_rvv(const uint8_t* input, int16_t* Gx, int16_t* Gy, int width, int height) {
    
    // Step 5: Border handling - strictly clear the entire output arrays to 0
    memset(Gx, 0, width * height * sizeof(int16_t));
    memset(Gy, 0, width * height * sizeof(int16_t));

    // Process the image avoiding the 2-pixel outermost padding
    // Changed from y=1 to y=2, and height-1 to height-2
    for (int y = 2; y < height - 2; ++y) {
        int x = 2;                  // Changed from 1 to 2
        int remain = width - 4;     // Changed from width-2 to width-4 (skipping 2 left, 2 right)

        // Precalculate Row Pointers to remove redundant arithmetic
        const uint8_t* row_top = input + (y - 1) * width;
        const uint8_t* row_mid = input + y * width;
        const uint8_t* row_bot = input + (y + 1) * width;

        int16_t* gx_row = Gx + y * width;
        int16_t* gy_row = Gy + y * width;

        while (remain > 0) {
            // (1) What it does: Requests vector length (vl) for 8-bit elements.
            // (2) Why this LMUL: LMUL=2 (m2) processes twice as many pixels per loop iteration compared to m1. We can safely do this now because row-by-row compute reduces register pressure.
            // (3) What changes if VLEN differs: 'vl' automatically scales up or down based on hardware width.
            size_t vl = __riscv_vsetvl_e8m2(remain);

            // =====================================================================
            // 1. Process TOP ROW (Load, Widen, Accumulate, Release Registers)
            // =====================================================================
            const uint8_t* tl_ptr = row_top + (x - 1);
            const uint8_t* tc_ptr = row_top + x;       
            const uint8_t* tr_ptr = row_top + (x + 1); 

            // (1) What it does: Loads 'vl' contiguous 8-bit pixels from memory.
            // (2) Why this LMUL: LMUL=2 to match the vsetvl configuration.
            // (3) What changes if VLEN differs: Loads more or fewer pixels natively.
            vuint8m2_t v_tl_8 = __riscv_vle8_v_u8m2(tl_ptr, vl);
            vuint8m2_t v_tc_8 = __riscv_vle8_v_u8m2(tc_ptr, vl);
            vuint8m2_t v_tr_8 = __riscv_vle8_v_u8m2(tr_ptr, vl);

            // (1) What it does: Zero-extends unsigned 8-bit to unsigned 16-bit, then safely reinterprets as signed 16-bit.
            // (2) Why this LMUL: LMUL=4 is required because widening doubles the data size (m2 -> m4).
            // (3) What changes if VLEN differs: Scales widening operation proportionally.
            vint16m4_t v_tl = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_tl_8, vl));
            vint16m4_t v_tc = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_tc_8, vl));
            vint16m4_t v_tr = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_tr_8, vl));

            // Start Gx Accumulation: Gx = TR - TL
            // (1) What it does: Performs vector subtraction for signed 16-bit elements.
            // (2) Why this LMUL: LMUL=4 because operands are 16-bit.
            vint16m4_t v_gx = __riscv_vsub_vv_i16m4(v_tr, v_tl, vl);
            
            // Start Gy Accumulation: Gy needs to subtract the top row (TL + 2*TC + TR)
            vint16m4_t v_tc_x2 = __riscv_vsll_vx_i16m4(v_tc, 1, vl); // Multiply by 2 via shift
            vint16m4_t v_gy_top = __riscv_vadd_vv_i16m4(v_tl, v_tr, vl);
            v_gy_top = __riscv_vadd_vv_i16m4(v_gy_top, v_tc_x2, vl);
            // Notice: We don't need the Top Row 8-bit/16-bit vectors anymore! Registers Freed!

            // =====================================================================
            // 2. Process MID ROW
            // =====================================================================
            const uint8_t* ml_ptr = row_mid + (x - 1);
            const uint8_t* mr_ptr = row_mid + (x + 1);

            vuint8m2_t v_ml_8 = __riscv_vle8_v_u8m2(ml_ptr, vl);
            vuint8m2_t v_mr_8 = __riscv_vle8_v_u8m2(mr_ptr, vl);

            vint16m4_t v_ml = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_ml_8, vl));
            vint16m4_t v_mr = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_mr_8, vl));

            // Accumulate Gx: Gx += 2 * (MR - ML)
            vint16m4_t v_gx_mid = __riscv_vsub_vv_i16m4(v_mr, v_ml, vl);
            v_gx_mid = __riscv_vsll_vx_i16m4(v_gx_mid, 1, vl); 
            v_gx = __riscv_vadd_vv_i16m4(v_gx, v_gx_mid, vl);

            // =====================================================================
            // 3. Process BOT ROW
            // =====================================================================
            const uint8_t* bl_ptr = row_bot + (x - 1);
            const uint8_t* bc_ptr = row_bot + x;       
            const uint8_t* br_ptr = row_bot + (x + 1); 

            vuint8m2_t v_bl_8 = __riscv_vle8_v_u8m2(bl_ptr, vl);
            vuint8m2_t v_bc_8 = __riscv_vle8_v_u8m2(bc_ptr, vl);
            vuint8m2_t v_br_8 = __riscv_vle8_v_u8m2(br_ptr, vl);

            vint16m4_t v_bl = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_bl_8, vl));
            vint16m4_t v_bc = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_bc_8, vl));
            vint16m4_t v_br = __riscv_vreinterpret_v_u16m4_i16m4(__riscv_vwcvtu_x_x_v_u16m4(v_br_8, vl));

            // Accumulate Gx: Gx += (BR - BL)
            vint16m4_t v_gx_bot = __riscv_vsub_vv_i16m4(v_br, v_bl, vl);
            v_gx = __riscv_vadd_vv_i16m4(v_gx, v_gx_bot, vl);

            // Accumulate Gy: Gy = (BL + 2*BC + BR) - Top_Row_Sum
            // Mathematically equivalent to: (BL - TL) + 2(BC - TC) + (BR - TR)
            vint16m4_t v_bc_x2 = __riscv_vsll_vx_i16m4(v_bc, 1, vl);
            vint16m4_t v_gy_bot = __riscv_vadd_vv_i16m4(v_bl, v_br, vl);
            v_gy_bot = __riscv_vadd_vv_i16m4(v_gy_bot, v_bc_x2, vl);
            
            vint16m4_t v_gy = __riscv_vsub_vv_i16m4(v_gy_bot, v_gy_top, vl);

            // =====================================================================
            // 7. Store results to memory
            // =====================================================================
            
            // (1) What it does: Stores 'vl' 16-bit elements back to memory (SoA layout).
            // (2) Why this LMUL: LMUL=4 to match our 16-bit accumulators.
            // (3) What changes if VLEN differs: The size of the memory write adjusts perfectly with VLEN.
            __riscv_vse16_v_i16m4(gx_row + x, v_gx, vl);
            __riscv_vse16_v_i16m4(gy_row + x, v_gy, vl);

            // Move forward by the number of elements processed (vl)
            x += vl;
            remain -= vl;
        }
    }
}