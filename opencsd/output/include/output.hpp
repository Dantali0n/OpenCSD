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

#ifndef QEMU_CSD_OUTPUT_HPP
#define QEMU_CSD_OUTPUT_HPP

#include <iostream>
#include <string>

#ifdef QEMUCSD_DEBUG
    #include <backward.hpp>
    using namespace backward;
#endif

namespace qemucsd::output {

    enum output_levels {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3,
        FATAL = 4
    };

    class Output {
    protected:
        static constexpr output_levels DEFAULT_LEVEL =
            #ifdef QEMUCSD_DEBUG
                DEBUG;
            #else
                INFO;
            #endif

        std::string prefix;
        output_levels current_level;

        template<typename I>
        static void output_i(std::ostream &out, I &&t);

        template<typename HeadI, typename... TailI>
        static void output_i(std::ostream &out, HeadI &&head, TailI&&... tail);
    public:
        Output(std::string prefix);

        Output(std::string prefix, output_levels level);

        template<typename T>
        void operator()(std::ostream &out, T &&t);

        template<typename Head, typename... Tail>
        void operator()(std::ostream &out, Head &&head, Tail&&... tail);

        template<typename T>
        void operator()(std::ostream &out, output_levels level, T &&t);

        template<typename Head, typename... Tail>
        void operator()(std::ostream &out, output_levels level, Head &&head, Tail&&... tail);

        template<typename T>
        void output(std::ostream &out, output_levels level, T &&t);

        template<typename Head, typename... Tail>
        void output(std::ostream &out, output_levels level, Head &&head, Tail&&... tail);

        template<typename D>
        void debug(D &&t);

        template<typename HeadD, typename... TailD>
        void debug(HeadD &&head, TailD&&... tail);

        template<typename T>
        void info(T &&t);

        template<typename Head, typename... Tail>
        void info(Head &&head, Tail&&... tail);

        template<typename T>
        void warning(T &&t);

        template<typename Head, typename... Tail>
        void warning(Head &&head, Tail&&... tail);

        template<typename T>
        void error(T &&t);

        template<typename Head, typename... Tail>
        void error(Head &&head, Tail&&... tail);

        template<typename F>
        void fatal(F &&t);

        template<typename HeadF, typename... TailF>
        void fatal(HeadF &&head, TailF&&... tail);
    };

    #include "output.tpp"

}

#endif // QEMU_CSD_OUTPUT_HPP
