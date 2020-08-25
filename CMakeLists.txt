#
# IODA
# Copyright and License: see LICENSE

cmake_minimum_required( VERSION 3.12 )
project( ioda LANGUAGES CXX Fortran )

## Ecbuild integration
find_package( ecbuild QUIET )
include( ecbuild_system NO_POLICY_SCOPE )
ecbuild_declare_project()
list( APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )
set( CMAKE_DIRECTORY_LABELS ${PROJECT_NAME} )

## Configuration options
include( ${PROJECT_NAME}_compiler_flags )

## Dependencies

# Required
find_package( NetCDF REQUIRED COMPONENTS C )
find_package( fckit REQUIRED )
find_package( oops REQUIRED )

## Sources
add_subdirectory( src )
add_subdirectory( test )
add_subdirectory( tools )

## Tests
ecbuild_add_test( TARGET ${PROJECT_NAME}_coding_norms
                  TYPE SCRIPT
                  COMMAND ${PROJECT_NAME}_cpplint.py
                  ARGS --quiet --recursive ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/test
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

## Package Config
ecbuild_install_project( NAME ${PROJECT_NAME} )

ecbuild_print_summary()
