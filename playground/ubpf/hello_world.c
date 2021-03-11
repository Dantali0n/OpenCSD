#include <stdint.h>

struct bpf_data {
	uint64_t data;
};

// Register asm `call 1` as bpf_register_test function
static void (*bpf_register_test)(void) = (void *) 1;

static uint64_t (*bpf_ktime_get_ns)(void) = (void *) 2;

static void (*bpf_struct_test)(struct bpf_data *data) = (void *) 3;

int main() {

	bpf_register_test();

	uint64_t test = bpf_ktime_get_ns();

	struct bpf_data data;

	bpf_struct_test(&data);

	return data.data;
}