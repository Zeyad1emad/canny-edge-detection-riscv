#include "magnitude_rvv.h"
#include <riscv_vector.h>
#include <cstdlib>

void compute_magnitude_rvv(const int16_t* gx, const int16_t* gy,
                            uint8_t* output, int width, int height) {
    int n = width * height;

    for (int i = 0; i < n; ) {
        // Get vector length for this iteration (VLA - never hardcodes VLEN)
        // Using e16m2: 16-bit elements, LMUL=2 (doubles throughput vs m1)
        size_t vl = __riscv_vsetvl_e16m2(n - i);

        // Step 1: Load Gx and Gy as int16_t vectors
        // vle16: vector load 16-bit elements, m2 because LMUL=2
        vint16m2_t vgx = __riscv_vle16_v_i16m2(gx + i, vl);
        vint16m2_t vgy = __riscv_vle16_v_i16m2(gy + i, vl);

        // Step 2: Compute absolute values |Gx| and |Gy|
        // vmax of (x, -x) gives |x| for signed integers
        vint16m2_t vgx_neg = __riscv_vneg_v_i16m2(vgx, vl);
        vint16m2_t vgy_neg = __riscv_vneg_v_i16m2(vgy, vl);
        vint16m2_t vax = __riscv_vmax_vv_i16m2(vgx, vgx_neg, vl); // |Gx|
        vint16m2_t vay = __riscv_vmax_vv_i16m2(vgy, vgy_neg, vl); // |Gy|

        // Step 3: Find max and min of |Gx|, |Gy|
        // vmax/vmin: element-wise max/min, m2 output matches m2 input
        vint16m2_t vmax = __riscv_vmax_vv_i16m2(vax, vay, vl);
        vint16m2_t vmin = __riscv_vmin_vv_i16m2(vax, vay, vl);

        // Step 4: Alpha-Max-Beta-Min approximation
        // Mag ≈ max + (min * 13) >> 5
        // 13/32 ≈ 0.406 ≈ beta in the approximation
        // vmul_vx: multiply vector by scalar (13), m2 stays m2
        vint16m2_t vmin_scaled = __riscv_vmul_vx_i16m2(vmin, 13, vl);
        // vsra_vx: arithmetic right shift by 5 (divide by 32)
        vint16m2_t vmin_shifted = __riscv_vsra_vx_i16m2(vmin_scaled, 5, vl);
        // vadd: add max + scaled_min to get final approximation
        vint16m2_t vmag = __riscv_vadd_vv_i16m2(vmax, vmin_shifted, vl);

        // Step 5: Narrow and saturate to uint8_t [0, 255]
        // vnclipu: narrowing with unsigned saturation
        // Input is m2 (int16), output is m1 (uint8) - LMUL halves on narrow
        // shift=0 means no additional shift before narrowing
        vuint8m1_t vout = __riscv_vnclipu_wx_u8m1(
            __riscv_vreinterpret_v_i16m2_u16m2(vmag), 0, __RISCV_VXRM_RNU, vl);

        // Step 6: Store result
        // vse8: vector store 8-bit elements, m1 output
        __riscv_vse8_v_u8m1(output + i, vout, vl);

        i += vl;
    }
}
