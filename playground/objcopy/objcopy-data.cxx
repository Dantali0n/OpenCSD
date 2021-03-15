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

#include <cstdint>
#include <iostream>
#include <CL/cl.hpp>

/** Because writing multi-line strings as code is insanity */
/*https://www.linuxjournal.com/content/embedding-file-executable-aka-hello-world-version-5967 */
/* here "data" comes from the filename data.o */
extern "C" char _binary_data_cl_start;
extern "C" char _binary_data_cl_end;

constexpr uint64_t NUM_BINS = 10;

int main() {

	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);
	if(all_platforms.size() == 0) {
		std::cout << " No platforms found. Check OpenCL installation!\n";
		exit(1);
	}
	cl::Platform default_platform=all_platforms[0];
	std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

	std::vector<cl::Device> all_devices;
	default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
	if(all_devices.size() == 0) {
		std::cout << " No devices found. Check OpenCL installation!\n";
		exit(1);
	}
	cl::Device default_device = all_devices[0];
	std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << std::endl;

	cl::Context context({default_device});
	cl::Program::Sources sources;
	sources.push_back({&_binary_data_cl_start, (&_binary_data_cl_end - &_binary_data_cl_start)});

	cl::Program program(context, sources);
	if(program.build({default_device}) != CL_SUCCESS) {
		std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
		exit(1);
	}

	cl::Buffer buffer_A(context, CL_MEM_READ_WRITE, sizeof(int)*NUM_BINS);
	cl::Buffer buffer_B(context, CL_MEM_READ_WRITE, sizeof(int)*NUM_BINS);
	cl::Buffer buffer_C(context, CL_MEM_READ_WRITE, sizeof(int)*NUM_BINS);

	int A[NUM_BINS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	int B[NUM_BINS] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0};

	cl::CommandQueue queue(context, default_device);

	queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, sizeof(int)*NUM_BINS, A);
	queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, sizeof(int)*NUM_BINS, B);

	cl::Kernel kernel_add = cl::Kernel(program, "simple_add");
	kernel_add.setArg(0, buffer_A);
	kernel_add.setArg(1, buffer_B);
	kernel_add.setArg(2, buffer_C);
	queue.enqueueNDRangeKernel(kernel_add, cl::NullRange, cl::NDRange(NUM_BINS), cl::NullRange);
	queue.finish();

	int C[NUM_BINS];
	queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, sizeof(int)*NUM_BINS, C);

	std::cout << " result: " << std::endl;
	for(uint64_t i = 0; i < NUM_BINS; i++){
		std::cout << C[i] << " ";
	}

	return 0;
}