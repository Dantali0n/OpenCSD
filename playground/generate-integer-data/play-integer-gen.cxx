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
#include <fstream>

using std::ios_base;

static constexpr uint32_t DATA_SIZE = 131072;
static const std::string OUTPUT_FILE = "integer.dat";

int main(int argc, char **argv) {
    std::ofstream out(OUTPUT_FILE, ios_base::out | ios_base::binary);

    char* data = new char[DATA_SIZE];

    uint32_t num_ints = DATA_SIZE / sizeof(uint32_t);
    uint32_t* int_alias = (uint32_t*) data;
    for(uint32_t i = 0; i < num_ints; i++) {
        *(int_alias + i) = rand() % UINT32_MAX;
    }

    out.write(data, DATA_SIZE);
    out.close();
}