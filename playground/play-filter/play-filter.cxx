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

#include <chrono>
#include <iostream>
#include <fstream>

using std::ios_base;

static constexpr uint32_t UINT32_HALF = RAND_MAX / 2;
static const std::string INPUT_FILE = "data.dat";

int main(int argc, char **argv) {
    std::ifstream in(INPUT_FILE, ios_base::in | ios_base::binary);

    in.seekg(0, ios_base::end);
    std::streamsize length = in.tellg();
    in.seekg(0, ios_base::beg);

    if(length < 0) {
        std::cerr << "File data.dat could not be found" << std::endl;
        exit(1);
    }

    char* data = new char[length];
    in.read(data, length);

    auto start = std::chrono::high_resolution_clock::now();

    uint64_t filter_count = 0;
    uint32_t* int_alias = (uint32_t*) data;
    uint64_t int_count = length / sizeof(uint32_t);
    for(uint64_t i = 0; i < int_count; i++) {
        if(*(int_alias + i) > UINT32_HALF) filter_count++;
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "Filter integers: " << duration.count() << " ms." << std::endl;

    std::cout << "nvme cli result: " << filter_count << std::endl;
}