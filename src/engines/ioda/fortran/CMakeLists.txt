# (C) Copyright 2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

list(APPEND SRCS_FORTRAN_BINDINGS
       ioda_f_c_string_mod.f90
       ioda_vecstring_mod.f90
       ioda_dimensions_mod.f90
       ioda_attribute_mod.f90
       ioda_has_attributes_mod.f90
       ioda_variable_creation_parameters_mod.f90 
       ioda_variable_mod.f90
       ioda_has_variables_mod.f90
       ioda_group_mod.f90
       ioda_engines_mod.f90
	)

ecbuild_add_library( TARGET ioda_fortran
                     SOURCES ${SRCS_FORTRAN_BINDINGS}
                     LINKER_LANGUAGE Fortran )
add_library(ioda::ioda_fortran ALIAS ioda_fortran)

set_target_properties(ioda_fortran PROPERTIES Fortran_PREPROCESS ON)
if (CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
	target_compile_options(ioda_fortran PRIVATE -ffree-line-length-none)
endif()

#Fortran module output directory for build and install interfaces
set(MODULE_DIR module/${PROJECT_NAME}/${CMAKE_Fortran_COMPILER_ID}/${CMAKE_Fortran_COMPILER_VERSION})
set_target_properties(ioda_fortran PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/${MODULE_DIR})
install(DIRECTORY ${CMAKE_BINARY_DIR}/${MODULE_DIR}/ DESTINATION ${MODULE_DIR})
target_include_directories(ioda_fortran INTERFACE
                                $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${MODULE_DIR}>
                                $<INSTALL_INTERFACE:${MODULE_DIR}>)

target_link_libraries(ioda_fortran PUBLIC ioda_engines stdc++)
AddLib(ioda_fortran SHARED ioda_fortran ${PROJECT_NAME}-targets)

target_include_directories(ioda_fortran PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${MODULE_DIR}>
    )

#INSTALL(DIRECTORY include/ioda_fortran
#	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#	COMPONENT Headers)
