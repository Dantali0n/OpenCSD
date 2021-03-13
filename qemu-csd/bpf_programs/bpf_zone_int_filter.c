//#include <linux/bpf.h>
#include <stdint.h>

#include "bpf_helpers_prog.h"

#define RAND_MAX 2147483647

/** Limitation examples, globals
 * int test; -> Clang won't compile
 * int test = 12; -> uBPF won't run, BPF relocation type 1
 */

// Global vars must be static const,
// uBPF does not support BPF relocation instructions!
static const uint16_t BUFFER_SIZE = 64;

int main() {
	uint64_t lba_size = bpf_get_lba_siza();
	uint8_t buffer[BUFFER_SIZE] = {0};

	uint16_t its_per_lba = lba_size / BUFFER_SIZE;
	uint32_t ints_per_it = BUFFER_SIZE / sizeof(uint32_t);

	uint64_t count = 0;

	for(uint16_t i = 0; i < its_per_lba; i++) {
		bpf_read(0, i*BUFFER_SIZE, BUFFER_SIZE, &buffer);

		uint32_t *int_buf = (uint32_t*)buffer;
		for(uint32_t j = 0; j < ints_per_it; j++) {
			if(*(int_buf + j) > RAND_MAX / 2) count++;
		}
	}

	bpf_return_data(&count, sizeof(uint64_t));

//	__u16 lba_size = 8;
//	__u8 buffer[8] = {0};
//
//	for(__u8 i = 0; i < sizeof(__u64); i++) {
//		buffer[i] = 1;
//	}
//
//	bpf_return_data(&buffer, test);

	return 0;
}