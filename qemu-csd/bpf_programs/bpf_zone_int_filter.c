//#include <linux/bpf.h>
#include <stdint.h>

#include "bpf_helpers_prog.h"

/** Limitation examples
 * int test; -> Clang won't compile
 * int test = 12; -> uBPF won't run, BPF relocation type 1
 */

// Global vars must be static const,
// uBPF does not support BPF relocation instruction!
static const uint16_t BUFFER_SIZE = 128;

int main() {
	uint64_t lba_size = bpf_get_lba_siza();
	uint8_t buffer[BUFFER_SIZE] = {0};

//	for(uint8_t i = 0; i < sizeof(uint64_t); i++) {
//		buffer[i] = 1;
//	}
	bpf_read(0, 0, BUFFER_SIZE, &buffer);

	bpf_return_data(&buffer, sizeof(uint64_t));

//	__u16 lba_size = 8;
//	__u8 buffer[8] = {0};
//
//	for(__u8 i = 0; i < sizeof(__u64); i++) {
//		buffer[i] = 1;
//	}
//
//	bpf_return_data(&buffer, test);

	return 12;
}