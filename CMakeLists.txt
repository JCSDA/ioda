#
# (C) Copyright 2017-2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

################################################################################
# IODA
################################################################################

cmake_minimum_required( VERSION 3.14 )
option(IODA_BUILD_LANGUAGE_FORTRAN "Build Fortran bindings." ON)
list( APPEND IODA_LANGUAGES CXX)
if (IODA_BUILD_LANGUAGE_FORTRAN)
	list( APPEND IODA_LANGUAGES Fortran)
endif()

project( ioda VERSION 2.9.0 LANGUAGES ${IODA_LANGUAGES} )

include( FeatureSummary )

option(OPENMP "Build ioda with OpenMP support" ON)

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
## Note: when updating these lists, please also update ioda-import.cmake.in.

# Required
if(OPENMP)
  find_package( OpenMP COMPONENTS C CXX Fortran )
endif()

set(HDF5_PREFER_PARALLEL true) # CMake sometimes mistakenly finds a serial system-provided HDF5.
find_package( HDF5 REQUIRED COMPONENTS C HL )
find_package( MPI REQUIRED )
find_package( jedicmake REQUIRED )
find_package( eckit 1.23.0 REQUIRED )
find_package( fckit 0.10.1 REQUIRED )
find_package( oops REQUIRED )
find_package( Eigen3 REQUIRED NO_MODULE HINTS
              $ENV{Eigen3_ROOT} $ENV{EIGEN3_ROOT} $ENV{Eigen_ROOT} $ENV{EIGEN_ROOT}
              $ENV{Eigen3_PATH} $ENV{EIGEN3_PATH} $ENV{Eigen_PATH} $ENV{EIGEN_PATH} )
find_package( gsl-lite REQUIRED HINTS $ENV{gsl_lite_DIR} )
find_package( udunits 2.2.0 REQUIRED )
find_package( NetCDF REQUIRED COMPONENTS Fortran )  # No idea why oops is not importing this correctly
find_package( Boost 1.64.0 REQUIRED )

# Optional
find_package( odc 1.4.6 QUIET )   # Needed for odc
# Python-finding settings
set(Python3_FIND_REGISTRY "LAST")
set(Python3_FIND_FRAMEWORK "LAST")
find_package( Python3 COMPONENTS Interpreter Development)
find_package( pybind11 QUIET)

if (pybind11_FOUND)
  option(BUILD_PYTHON_BINDINGS "Build Python bindings using pybind11?" ON)
else()
  option(BUILD_PYTHON_BINDINGS "Build Python bindings using pybind11?" OFF)
endif()
add_feature_info(Python BUILD_PYTHON_BINDINGS "Build Python bindings using pybind11")

## Test data

# DH 2023/10/06 - this is unfortunately still true ...
### The ioda_data package is problematic in bundles because it has some special CMake logic
### at the root bundle location. So, we cannot find the ioda data while in ioda!
### Basically, with the current logic we hack ioda-data_FOUND to match ENABLE_IODA_DATA
#find_package( ioda-data QUIET )
if (ENABLE_IODA_DATA)
  set (ioda-data_FOUND TRUE)
else()
  # Set values for downloading ioda test data from tarball
  set (ioda_data_download_url "https://bin.ssec.wisc.edu/pub/s4/CRTM/file")
  set (ioda_data_tag "2.9.0")
  set (ioda_data_tarball_name "ioda_testinput_tier_1_${ioda_data_tag}.tar.gz")
  set (ioda_data_tarball_hash "94fc90a04f6587b9a07607dfadb04d94")
endif()

## Sources
add_subdirectory( share )         # no arch data
add_subdirectory( src/engines )   # `ioda_engines` target
add_subdirectory( src )           # `ioda` target
add_subdirectory( src/mains )     # ioda executable targets
if ( Python3_FOUND )
    add_subdirectory( src/python ) # ioda python targets
endif()
add_subdirectory( test )          # ioda ctests
add_subdirectory( tools )         # misc tools

if( ENABLE_IODA_DOC )
    add_subdirectory( docs )
endif()

## Tests
ecbuild_add_test( TARGET ${PROJECT_NAME}_coding_norms
                  TYPE SCRIPT
                  COMMAND ${PROJECT_NAME}_cpplint.py
                  ARGS --quiet --recursive ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/test
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set_tests_properties( ${PROJECT_NAME}_coding_norms PROPERTIES TIMEOUT 90 )

## Package Config
ecbuild_install_project( NAME ${PROJECT_NAME} )

ecbuild_print_summary()

