#include "angle_rvv.h"
#include <riscv_vector.h>
#include <cstdlib>

void compute_direction_rvv(const int16_t* Gx, const int16_t* Gy,
                            uint8_t* output, int width, int height) {
    int total = width * height;
    int i = 0;

    while (i < total) {
        size_t vl = __riscv_vsetvl_e16m2(total - i);

        vint16m2_t vgx = __riscv_vle16_v_i16m2(Gx + i, vl);
        vint16m2_t vgy = __riscv_vle16_v_i16m2(Gy + i, vl);

        vint16m2_t ax = __riscv_vabs_v_i16m2(vgx, vl);
        vint16m2_t ay = __riscv_vabs_v_i16m2(vgy, vl);

        vint16m2_t ay5  = __riscv_vmul_vx_i16m2(ay,  5, vl);
        vint16m2_t ax2  = __riscv_vmul_vx_i16m2(ax,  2, vl);
        vint16m2_t ax12 = __riscv_vmul_vx_i16m2(ax, 12, vl);

        vbool8_t mask_is0 = __riscv_vmslt_vv_i16m2_b8(ay5, ax2,  vl);
        vbool8_t mask_is1 = __riscv_vmslt_vv_i16m2_b8(ay5, ax12, vl);
        vbool8_t mask_is2 = __riscv_vmslt_vv_i16m2_b8(ax12, ay5, vl);

        vuint8m1_t dir = __riscv_vmv_v_x_u8m1(3, vl);
        dir = __riscv_vmerge_vxm_u8m1(dir, 2, mask_is2, vl);
        dir = __riscv_vmerge_vxm_u8m1(dir, 1, mask_is1, vl);
        dir = __riscv_vmerge_vxm_u8m1(dir, 0, mask_is0, vl);

        __riscv_vse8_v_u8m1(output + i, dir, vl);

        i += vl;
    }
}
