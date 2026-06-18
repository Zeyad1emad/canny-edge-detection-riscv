#include "direction_rvv.h"
#include <riscv_vector.h>

void compute_direction_rvv(const int16_t* Gx, const int16_t* Gy, uint8_t* output, int width, int height) {
    size_t n = (size_t)width * height;
    
    while (n > 0) {
        size_t vl = __riscv_vsetvl_e16m2(n);

        // Load Gx and Gy
        vint16m2_t v_gx = __riscv_vle16_v_i16m2(Gx, vl);
        vint16m2_t v_gy = __riscv_vle16_v_i16m2(Gy, vl);

        // Compute absolute values: max(v, -v)
        vint16m2_t v_gx_neg = __riscv_vrsub_vx_i16m2(v_gx, 0, vl);
        vint16m2_t v_ax = __riscv_vmax_vv_i16m2(v_gx, v_gx_neg, vl);

        vint16m2_t v_gy_neg = __riscv_vrsub_vx_i16m2(v_gy, 0, vl);
        vint16m2_t v_ay = __riscv_vmax_vv_i16m2(v_gy, v_gy_neg, vl);

        // Widening multiplication to 32-bit to prevent overflow
        vint32m4_t v_ay5  = __riscv_vwmul_vx_i32m4(v_ay, 5, vl);
        vint32m4_t v_ax2  = __riscv_vwmul_vx_i32m4(v_ax, 2, vl);
        vint32m4_t v_ax12 = __riscv_vwmul_vx_i32m4(v_ax, 12, vl);

        // Initialize output vector with default direction 3
        vuint8m1_t v_dir = __riscv_vmv_v_x_u8m1(3, vl);

        // if (ay * 5 > ax * 12) dir = 2
        vbool8_t mask_gt12 = __riscv_vmsgt_vv_i32m4_b8(v_ay5, v_ax12, vl);
        v_dir = __riscv_vmerge_vxm_u8m1(v_dir, 2, mask_gt12, vl);

        // if (ay * 5 < ax * 12) dir = 1
        vbool8_t mask_lt12 = __riscv_vmslt_vv_i32m4_b8(v_ay5, v_ax12, vl);
        v_dir = __riscv_vmerge_vxm_u8m1(v_dir, 1, mask_lt12, vl);

        // if (ay * 5 < ax * 2) dir = 0
        vbool8_t mask_lt2 = __riscv_vmslt_vv_i32m4_b8(v_ay5, v_ax2, vl);
        v_dir = __riscv_vmerge_vxm_u8m1(v_dir, 0, mask_lt2, vl);

        // Store quantized directions
        __riscv_vse8_v_u8m1(output, v_dir, vl);

        Gx += vl;
        Gy += vl;
        output += vl;
        n -= vl;
    }
}