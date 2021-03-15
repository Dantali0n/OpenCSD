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

/**
 * This file is based upon source files from generic-ebpf
 * See: https://github.com/generic-ebpf/generic-ebpf
 */

//// Be sure to include C dependencies through extern "C"
extern "C" {
	#include <stdint.h>
	#include <sys/ebpf.h>
	#include <sys/ebpf_vm_isa.h>

	#include "bpf_bootstrap_program.h"
}

enum test_emts {
	EBPF_MAP_TYPE_ARRAY,
	EBPF_MAP_TYPE_PERCPU_ARRAY,
	EBPF_MAP_TYPE_HASHTABLE,
	EBPF_MAP_TYPE_PERCPU_HASHTABLE,
	EBPF_MAP_TYPE_MAX
};

enum test_epts {
	EBPF_PROG_TYPE_TEST,
	EBPF_PROG_TYPE_MAX
};

enum test_ehts {
	EBPF_HELPER_TYPE_map_lookup_elem,
	EBPF_HELPER_TYPE_map_update_elem,
	EBPF_HELPER_TYPE_map_delete_elem,
	EBPF_HELPER_TYPE_MAX
};

static bool test_is_map_usable(struct ebpf_map_type *emt) {
	if (emt == &emt_array) return true;
	if (emt == &emt_percpu_array) return true;
	if (emt == &emt_hashtable) return true;
	if (emt == &emt_percpu_hashtable) return true;
	return false;
}

static bool test_is_helper_usable(struct ebpf_helper_type *eht) {
	if (eht == &eht_map_lookup_elem) return true;
	if (eht == &eht_map_update_elem) return true;
	if (eht == &eht_map_delete_elem) return true;
	return false;
}

static const struct ebpf_prog_type ept_test = {
	"test",
	{
		test_is_map_usable,
		test_is_helper_usable
	}
};

static const struct ebpf_preprocessor_type eppt_test = {
	"test",
	{ NULL }
};

static const struct ebpf_config ebpf_test_config = {
	.prog_types = {
		[EBPF_PROG_TYPE_TEST] = &ept_test
	},
	.map_types = {
		[EBPF_MAP_TYPE_ARRAY] = &emt_array,
		[EBPF_MAP_TYPE_PERCPU_ARRAY] = &emt_percpu_array,
		[EBPF_MAP_TYPE_HASHTABLE] = &emt_hashtable,
		[EBPF_MAP_TYPE_PERCPU_HASHTABLE] = &emt_percpu_hashtable
	},
	.helper_types = {
		[EBPF_HELPER_TYPE_map_lookup_elem] = &eht_map_lookup_elem,
		[EBPF_HELPER_TYPE_map_update_elem] = &eht_map_update_elem,
		[EBPF_HELPER_TYPE_map_delete_elem] = &eht_map_delete_elem
	},
	.preprocessor_type = &eppt_test
};

int main(int argc, char **argv) {
	struct bpf_bootstrap_program *skel;
	struct ebpf_prog *ep;
	struct ebpf_env *ee;

	int err;

	/* Open BPF application */
	skel = bpf_bootstrap_program__open();
	if (!skel) {
		fprintf(stderr, "Failed to open BPF skeleton\n");
		return 1;
	}

	err = ebpf_env_create(&ee, &ebpf_test_config);

	// TODO(Dantali0n): convert skel->skeleton->data to ebpf_inst
	struct ebpf_inst insts[] = {{EBPF_OP_EXIT, 0, 0, 0, 0}};

	struct ebpf_prog_attr attr = {
		.type = EBPF_PROG_TYPE_TEST,
		.prog = insts,
		.prog_len = 1 //(uint32_t) skel->skeleton->data_sz
	};

	err = ebpf_prog_create(ee, &ep, &attr);

	err = ebpf_prog_run(NULL, ep);
}