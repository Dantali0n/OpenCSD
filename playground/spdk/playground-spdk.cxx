#include <iostream>
#include <spdk/stdinc.h>

#include "spdk/nvme.h"
#include "spdk/vmd.h"
#include "spdk/nvme_zns.h"
#include "spdk/env.h"

int main(int argc, char* argv[]) {
	int rc;
	struct spdk_env_opts opts;

	spdk_env_opts_init(&opts);

	opts.name = "hello_world";
	opts.shm_id = 0;
	if (spdk_env_init(&opts) < 0) {
		fprintf(stderr, "Unable to initialize SPDK env\n");
		return 1;
	}
}