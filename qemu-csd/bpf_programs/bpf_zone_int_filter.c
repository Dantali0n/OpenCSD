#include <stdint.h>

#include "bpf_helpers_prog.h"

int main() {

	uint64_t test = 12;

	bpf_return_data(&test, sizeof(test));

	return 12;
}