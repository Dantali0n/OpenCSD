#include <iostream>

//// Be sure to include C dependencies through extern "C"
extern "C" {
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/resource.h>

//	#include <bpf/bpf_helpers.h>

	#include "bpf_bootstrap_program.h"
}

static int libbpf_print_fn(
	enum libbpf_print_level level, const char *format, va_list args
) {
	return vfprintf(stderr, format, args);
}

static void bump_memlock_rlimit() {
	struct rlimit rlim_new = {
			.rlim_cur	= RLIM_INFINITY,
			.rlim_max	= RLIM_INFINITY,
	};

	if (setrlimit(RLIMIT_MEMLOCK, &rlim_new)) {
		fprintf(stderr, "Failed to increase RLIMIT_MEMLOCK limit!\n");
		exit(1);
	}
}

int main(int argc, char **argv) {
	struct bpf_bootstrap_program *skel;
	int err;

	/* Set up libbpf errors and debug info callback */
	libbpf_set_print(libbpf_print_fn);

	/* Bump RLIMIT_MEMLOCK to allow BPF sub-system to do anything */
	bump_memlock_rlimit();

	/* Open BPF application */
	skel = bpf_bootstrap_program__open();
	if (!skel) {
		fprintf(stderr, "Failed to open BPF skeleton\n");
		return 1;
	}

	/* ensure BPF program only handles write() syscalls from our process */
	skel->bss->my_pid = getpid();

	/* Load & verify BPF programs */
	err = bpf_bootstrap_program__load(skel);
	if (err) {
		fprintf(stderr, "Failed to load and verify BPF skeleton\n");
		goto cleanup;
	}

	/* Attach tracepoint handler */
	err = bpf_bootstrap_program__attach(skel);
	if (err) {
		fprintf(stderr, "Failed to attach BPF skeleton\n");
		goto cleanup;
	}

	printf("Successfully started!\n");

	for (;;) {
		/* trigger our BPF program */
		fprintf(stderr, ".");
		sleep(1);
	}

cleanup:
	bpf_bootstrap_program__destroy(skel);
	return -err;

	return 0;
}