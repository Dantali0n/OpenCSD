#include <iostream>

//// Be sure to include C dependencies through extern "C"
extern "C" {
	#include <stdint.h>
	#include <ubpf.h>

//#include <linux/bpf.h>
//#include <bpf/bpf_helpers.h>

	#include "bpf_ubpf_program.h"
}

static void bpf_register_test(void) {
	std::cout << "BPF program called bpf_register_test" << std::endl;
}

int main(int argc, char **argv) {
	static constexpr uint64_t mem_size = 1024*512;
	struct bpf_ubpf_program *skel;
	struct ubpf_vm *vm;

	uint64_t result;
	int err;

	void *memory = malloc(mem_size);
	char *message = (char*) malloc(mem_size);

	/* Open BPF application */
	skel = bpf_ubpf_program__open();
	if (!skel) {
		fprintf(stderr, "Failed to open BPF skeleton\n");
		return 1;
	}

	vm = ubpf_create();

	ubpf_register(vm, 1, "bpf_register_test", (void*)bpf_register_test);

	err = ubpf_load_elf(vm, skel->skeleton->data, skel->skeleton->data_sz, &message);

	result =  ubpf_exec(vm, memory, mem_size);
}