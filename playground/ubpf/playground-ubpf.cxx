/**
 * MIT License
 *
 * Copyright (c) 2021 Dantali0n
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

#include <iostream>

//// Be sure to include C dependencies through extern "C"
extern "C" {
	#include <stdint.h>
	#include <ubpf.h>

//#include <linux/bpf.h>
//#include <bpf/bpf_helpers.h>

	#include "bpf_ubpf_program.h"
}

static uint64_t bpf_ktime_get_ns(void) {
	return 512;
}

static void bpf_register_test(void) {
	std::cout << "BPF program called bpf_register_test" << std::endl;
}

struct bpf_data {
	uint64_t data;
};

static void bpf_struct_test(struct bpf_data *data) {
	data->data = 12;
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
	ubpf_register(vm, 2, "bpf_ktime_get_ns", (void*)bpf_ktime_get_ns);
	ubpf_register(vm, 3, "bpf_struct_test", (void*)bpf_struct_test);

	err = ubpf_load_elf(vm, skel->skeleton->data, skel->skeleton->data_sz, &message);

	uint64_t return_code;
	result = ubpf_exec(vm, memory, mem_size, &return_code);
}