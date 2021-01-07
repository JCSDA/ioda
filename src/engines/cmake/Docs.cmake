# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
if(NOT BUILD_DOCUMENTATION)
	set(BUILD_DOCUMENTATION "No" CACHE STRING "Build and/or install Doxygen-produced documentation?" FORCE)
	set_property(CACHE BUILD_DOCUMENTATION PROPERTY STRINGS No BuildOnly BuildAndInstall)
endif()

if(BUILD_DOCUMENTATION STREQUAL "No")
else()
    find_package(Doxygen)
    if (NOT DOXYGEN_FOUND)
        message(SEND_ERROR "Documentation build requested but Doxygen is not found.")
    endif()

    if (NOT DOXYGEN_DOT_EXECUTABLE)
	    set(HAVE_DOT NO)
    else()
	    set(HAVE_DOT YES)
    endif()


    configure_file(docs/Doxyfile.in
        "${PROJECT_BINARY_DIR}/Doxyfile" @ONLY)

	if(BUILD_DOCUMENTATION STREQUAL "BuildAndInstall")
        set (ALL_FLAG ALL)
    else()
        set (ALL_FLAG "")
    endif()

    # This builds the html docs
    add_custom_target(doc-html ${ALL_FLAG}
        ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Generating API html documentation with Doxygen" VERBATIM
    )
	set_target_properties( doc-html PROPERTIES FOLDER "Docs")
    # This builds the latex docs
    #    add_custom_target(doc-latex ${ALL_FLAG}
    #        latex refman.tex
    #        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs/latex
    #        COMMENT "Generating API pdf documentation with Doxygen" VERBATIM
    #    )

    add_custom_target(docs ${ALL_FLAG} DEPENDS doc-html)
	set_target_properties( docs PROPERTIES FOLDER "Docs")
endif()

if(BUILD_DOCUMENTATION STREQUAL "BuildAndInstall")
    # Provides html and pdf
    install(CODE "execute_process(COMMAND \"${CMAKE_BUILD_TOOL}\" docs)")
    # html
    install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/html/ DESTINATION ${INSTALL_DOC_DIR}/html COMPONENT Documentation)
    # pdf
    #    install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/latex/ DESTINATION ${INSTALL_DOC_DIR}/latex)
endif()
