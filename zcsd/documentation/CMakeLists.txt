# MIT License
#
# Copyright (c) 2021 Dantali0n
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

project(documentation)

find_package(LATEX)

# The directory where the final build data will be stored.
set(OUT_DIRECTORY "${CMAKE_BINARY_DIR}")
# Directory where the source files are located.
set(WORKINGDIR "${CMAKE_CURRENT_SOURCE_DIR}" )

# Latex source file.
set(MAIN_TEX "${CMAKE_CURRENT_SOURCE_DIR}/documentation.tex")

if(LATEX_FOUND AND LATEX_PDFLATEX_FOUND)
    # First pass.
    add_custom_target(latex-prebuild
        COMMAND texi2pdf -o ${OUT_DIRECTORY}/report.pdf -b ${MAIN_TEX}
        COMMENT "Starting Prebuild."
        WORKING_DIRECTORY ${WORKINGDIR}
        DEPENDS ${MAIN_TEX}
    )

    # Second pass - generate the final pdf.
    add_custom_target(latex-pdf
        COMMAND texi2pdf -o ${OUT_DIRECTORY}/report.pdf ${MAIN_TEX}
        WORKING_DIRECTORY ${WORKINGDIR}
        COMMENT "Assembling the final pdf file."
        DEPENDS latex-prebuild
    )

    add_dependencies(all-documents latex-pdf)
else()
    message(ERROR "No latex tools found!")
endif()
