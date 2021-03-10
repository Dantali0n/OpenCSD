// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2020 Facebook */

// Register asm `call 1` as bpf_register_test function
static void *(*bpf_register_test)(void) = (void *) 1;

int main() {

	bpf_register_test();

	return 12;
}