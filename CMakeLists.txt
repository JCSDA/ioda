#
# (C) Copyright 2017-2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

################################################################################
# IODA
################################################################################

cmake_minimum_required( VERSION 3.12 )
option(IODA_BUILD_LANGUAGE_FORTRAN "Build Fortran bindings." ON)
list( APPEND IODA_LANGUAGES CXX)
if (IODA_BUILD_LANGUAGE_FORTRAN)
	list( APPEND IODA_LANGUAGES Fortran)
endif()

project( ioda VERSION 1.0.0 LANGUAGES ${IODA_LANGUAGES} )

## Ecbuild integration
find_package( ecbuild QUIET )
include( ecbuild_system NO_POLICY_SCOPE )
ecbuild_declare_project()
list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )
set( CMAKE_DIRECTORY_LABELS ${PROJECT_NAME} )

## Configuration options
include( ${PROJECT_NAME}_compiler_flags )
option( ENABLE_IODA_DOC "Build IODA documentation" OFF )

## Dependencies

# Required
find_package( jedicmake QUIET )  # Prefer find modules from jedi-cmake
find_package( OpenMP COMPONENTS C CXX Fortran )
find_package( NetCDF REQUIRED COMPONENTS C )
find_package( eckit 1.11.6 REQUIRED )
find_package( fckit 0.7.0 REQUIRED )
find_package( oops 1.0.0 REQUIRED )

## Sources
add_subdirectory( src/engines ) # `ioda_engines` target
add_subdirectory( src )         # `ioda` target
add_subdirectory( src/mains )    # ioda executable targets
add_subdirectory( test )        # ioda ctests
add_subdirectory( tools )       # misc tools

if( ENABLE_IODA_DOC )
    add_subdirectory( docs )
endif()

## Tests
ecbuild_add_test( TARGET ${PROJECT_NAME}_coding_norms
                  TYPE SCRIPT
                  COMMAND ${PROJECT_NAME}_cpplint.py
                  ARGS --quiet --recursive ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/test
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

## Package Config
ecbuild_install_project( NAME ${PROJECT_NAME} )

ecbuild_print_summary()
