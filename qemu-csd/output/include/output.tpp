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

template<typename T>
void Output::operator()(std::ostream &out, T &&t) {
    output(out, &out == &std::cerr ? ERROR : INFO, t);
}

template<typename Head, typename... Tail>
void Output::operator()(std::ostream &out, Head &&head, Tail&&... tail) {
    output(out, &out == &std::cerr ? ERROR : INFO, head,
           std::forward<Tail>(tail)...);
}

template<typename T>
void Output::operator()(std::ostream &out, output_levels level, T &&t) {
    output(out, level, t);
}

template<typename Head, typename... Tail>
void Output::operator()(std::ostream &out, output_levels level, Head &&head, Tail&&... tail) {
    output(out, level, head, std::forward<Tail>(tail)...);
}

template<typename T>
void Output::output(std::ostream &out, output_levels level, T &&t) {
    if(level < current_level) return;
    out << prefix << t << "\n";
}

template<typename Head, typename... Tail>
void Output::output(std::ostream &out, output_levels level, Head &&head, Tail&&... tail) {
    if(level < current_level) return;
    out << prefix << head;
    output_i(out, std::forward<Tail>(tail)...);
}

template<typename T>
void Output::debug(T &&t) {
    output(std::cout, DEBUG, t);
}

template<typename Head, typename... Tail>
void Output::debug(Head &&head, Tail&&... tail) {
    output(std::cout, DEBUG, std::forward<Tail>(tail)...);
}

template<typename T>
void Output::info(T &&t) {
    output(std::cout, INFO, t);
}

template<typename Head, typename... Tail>
void Output::info(Head &&head, Tail&&... tail) {
    output(std::cout, INFO, std::forward<Tail>(tail)...);
}

template<typename T>
void Output::warning(T &&t) {
    output(std::cout, WARNING, t);
}

template<typename Head, typename... Tail>
void Output::warning(Head &&head, Tail&&... tail) {
    output(std::cout, WARNING, std::forward<Tail>(tail)...);
}

template<typename T>
void Output::error(T &&t) {
    output(std::cerr, ERROR, t);
}

template<typename Head, typename... Tail>
void Output::error(Head &&head, Tail&&... tail) {
    output(std::cerr, ERROR, std::forward<Tail>(tail)...);
}

template<typename T>
void Output::fatal(T &&t) {
    output(std::cerr, FATAL, t);
}

template<typename Head, typename... Tail>
void Output::fatal(Head &&head, Tail&&... tail) {
    output(std::cerr, FATAL, std::forward<Tail>(tail)...);
}

template<typename D>
void Output::output_i(std::ostream &out, D &&t) {
    out << t << "\n";
}

template<typename Head2, typename... Tail2>
void Output::output_i(std::ostream &out, Head2 &&head, Tail2&&... tail) {
    out << head;
    output_i(out, std::forward<Tail2>(tail)...);
}
