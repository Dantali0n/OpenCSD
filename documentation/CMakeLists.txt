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


    ADD_CUSTOM_TARGET(all-formats ALL) # Entry point of execution.
    ADD_DEPENDENCIES( all-formats latex-pdf)
else()
    message(ERROR "No latex tools found!")
endif()
