/**
 * Copyright 2019 David Calavera
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>

// Be sure to include C dependencies through extern "C"
extern "C" {
//	#include <stdio.h>

	#include <bpf_load.h>
	#include <trace_helpers.h>
}

int main(int argc, char **argv) {
	if (load_bpf_file("bpf_program.o") != 0) {
		printf("The kernel didn't load the BPF program\n");
		return -1;
	}

	read_trace_pipe();

	return 0;
}