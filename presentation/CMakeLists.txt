project(presentation)

find_package(LATEX)

# The directory where the final build data will be stored.
set(OUT_DIRECTORY "${CMAKE_BINARY_DIR}")
# Directory where the source files are located.
set(WORKINGDIR "${CMAKE_CURRENT_SOURCE_DIR}" )

# Latex source file.
set(MAIN_TEX "${CMAKE_CURRENT_SOURCE_DIR}/presentation.tex")

if(LATEX_FOUND AND LATEX_PDFLATEX_FOUND)
    # First pass.
    add_custom_target(latex-prebuild-presentation
        COMMAND texi2pdf -o ${OUT_DIRECTORY}/presentation.pdf -b ${MAIN_TEX}
        COMMENT "Starting Prebuild."
        WORKING_DIRECTORY ${WORKINGDIR}
        DEPENDS ${MAIN_TEX}
    )

    # Second pass - generate the final pdf.
    add_custom_target(latex-pdf-presentation
        COMMAND texi2pdf -o ${OUT_DIRECTORY}/presentation.pdf ${MAIN_TEX}
        WORKING_DIRECTORY ${WORKINGDIR}
        COMMENT "Assembling the final pdf file."
        DEPENDS latex-prebuild
    )

    add_dependencies(all-documents latex-pdf-presentation)
else()
    message(ERROR "No latex tools found!")
endif()
