/**
 * MIT License
 *
 * Copyright (c) 2022 Dantali0n
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//#include <linux/bpf.h>
#include <stdint.h>
#include <math.h>

#include <bpf_helpers_prog.h>
#include <bpf_helpers_flfs.h>

//#define RAND_MAX 2147483647
//
//typedef int int_fixed;
//
//#define FRACT_BITS 8
//#define FIXED_POINT_ONE (1 << FRACT_BITS)
//#define MAKE_INT_FIXED(x) ((x) << FRACT_BITS)
//#define MAKE_FLOAT_FIXED(x) ((int_fixed)((x) * FIXED_POINT_ONE))
//#define MAKE_FIXED_INT(x) ((x) >> FRACT_BITS)
//#define MAKE_FIXED_FLOAT(x) (((float)(x)) / FIXED_POINT_ONE)
//
//#define FIXEDPT_ONE	((int_fixed)((int_fixed)1 << FRACT_BITS))
//#define FIXEDPT_TWO	(FIXEDPT_ONE + FIXEDPT_ONE)
//
//#define MAKE_FIXED_RCONST(R) ((int_fixed)((R) * FIXEDPT_ONE + ((R) >= 0 ? 0.5 : -0.5)))
//
//int bdiv(int dividend , int divisor) {
//    int remainder = dividend ;
//    int quotient  = 0 ;
//
//    int i ;
//    for( i = 0 ; i < 17 ; i++ )
//    {
//        remainder = remainder - divisor ;
//        if( (remainder & 0x8000)  )
//        {
//            remainder = remainder + divisor ;
//            quotient  = quotient << 1 ;
//        }
//        else
//            quotient = (( quotient >> 1 ) | 0x1) ;
//
//        divisor = divisor >> 1;
//    }
//    return quotient ;
//}
//
//#define FIXED_MULT(x, y) ((x)*(y) >> FRACT_BITS)
//#define FIXED_DIV(x, y) (bdiv((x)<<FRACT_BITS, (y)))
//
//static inline int_fixed fixedpt_ln(int_fixed x) {
//    int_fixed log2, xi;
//    int_fixed f, s, z, w, R;
//    const int_fixed LN2 = MAKE_FIXED_RCONST(0.69314718055994530942);
//    const int_fixed LG[7] = {
//        MAKE_FIXED_RCONST(6.666666666666735130e-01),
//        MAKE_FIXED_RCONST(3.999999999940941908e-01),
//        MAKE_FIXED_RCONST(2.857142874366239149e-01),
//        MAKE_FIXED_RCONST(2.222219843214978396e-01),
//        MAKE_FIXED_RCONST(1.818357216161805012e-01),
//        MAKE_FIXED_RCONST(1.531383769920937332e-01),
//        MAKE_FIXED_RCONST(1.479819860511658591e-01)
//    };
//
//    if (x < 0)
//        return (0);
//    if (x == 0)
//        return 0xffffffff;
//
//    log2 = MAKE_FIXED_INT(0);
//    xi = x;
//    while (xi > FIXEDPT_TWO) {
//        xi >>= 1;
//        log2++;
//    }
//    f = xi - FIXEDPT_ONE;
//    s = FIXED_DIV(f, FIXEDPT_TWO + f);
//    z = FIXED_MULT(s, s);
//    w = FIXED_MULT(z, z);
//    R = FIXED_MULT(w, LG[1] + FIXED_MULT(w, LG[3]
//                    + FIXED_MULT(w, LG[5]))) + FIXED_MULT(z, LG[0]
//                    + FIXED_MULT(w, LG[2] + FIXED_MULT(w, LG[4]
//                    + FIXED_MULT(w, LG[6]))));
//    return (FIXED_MULT(LN2, (log2 << FRACT_BITS)) + f
//        - FIXED_MULT(s, f - R));
//}
//
//static inline int_fixed fixedpt_log(int_fixed x, int_fixed base) {
//    return (FIXED_DIV(fixedpt_ln(x), fixedpt_ln(base)));
//}

int main() {
    // Keep datastructures on the heap due to very limited stack space
    struct flfs_call *call = 0;
    bpf_get_call_info((void**)&call);

    uint64_t *cur_data_lba = (uint64_t*)call;
    find_data_lba((uint64_t**)&cur_data_lba, false);

    uint64_t zone_capacity = bpf_get_zone_capacity();
    uint64_t zone_size = bpf_get_zone_size();
    uint64_t sector_size = bpf_get_sector_size();

    if(call == 0) return -1;
    if(*cur_data_lba == 0) return -2;

    // Ensure the read kernel is being used for a read operation
    if(call->op == FLFS_READ_STREAM) return -3;

    uint64_t buffer_size;
    void *buffer;
    bpf_get_mem_info(&buffer, &buffer_size);

    uint64_t data_limit = call->dims.size < call->ino.size ?
        call->dims.size : call->ino.size;
    uint64_t buffer_offset = 0;
    uint64_t zone, sector = 0;
    uint64_t bytes_per_it = sector_size / sizeof(uint8_t);
    uint8_t *byte_buf = (uint8_t*)buffer;

    uint32_t *bins = (uint32_t*)(byte_buf + sector_size);
    for(uint16_t i = 0; i < 256; i++) {
        bins[i] = 0;
    }

    while(buffer_offset < data_limit) {
        lba_to_position(*cur_data_lba, zone_size, &zone, &sector);
        bpf_read(zone, sector, 0, sector_size, buffer);

        for(uint64_t j = 0; j < bytes_per_it; j++) {
            bins[*(byte_buf + j)] += 1;
        }

        buffer_offset = buffer_offset + sector_size;
        next_data_lba(&cur_data_lba);
    }

    bpf_return_data(bins, sizeof(uint32_t) * 256);

//    fixedpt base2 = fixedpt_fromint(2);
//    fixedpt entropy = 0;
//    fixedpt frequency = 0;
//    for(uint16_t i = 0; i < 256; i++) {
//        if(bins[i] > 0) {
//            fixedpt temp1 = fixedpt_fromint(bins[i]);
//            fixedpt temp2 = fixedpt_fromint((uint32_t)call->dims.size);
//            // Frequency can at most be 1
//            frequency = fixedpt_div(temp1, temp2);
//            entropy = fixedpt_add(entropy, fixedpt_mul(frequency, fixedpt_log(frequency, base2)));
//        }
//    }
//
//    entropy = fixedpt_sub(entropy, fixedpt_add(entropy, entropy));
    //entropy -= entropy;

//    bpf_return_data(&entropy, sizeof(float));

//    int_fixed base2 = MAKE_INT_FIXED(2);
//    int_fixed entropy = MAKE_INT_FIXED(0);
//    int_fixed frequency = MAKE_INT_FIXED(0);
//    for(uint16_t i = 0; i < 256; i++) {
//        if(bins[i] > 0) {
//            int_fixed temp1 = MAKE_INT_FIXED(bins[i]);
//            int_fixed temp2 = MAKE_INT_FIXED(call->dims.size);
//            // Frequency can at most be 1
//            frequency = FIXED_DIV(temp1, temp2);
//            entropy = entropy + FIXED_MULT(frequency, fixedpt_log(frequency, base2));
//        }
//    }
//
//    entropy -= entropy;
//
//    bpf_return_data(&entropy, sizeof(int));

    return 0;
}