#
# (C) Copyright 2017-2022 UCAR.
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

project( ioda VERSION 2.1.0 LANGUAGES ${IODA_LANGUAGES} )

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

# Required
if(OPENMP)
  find_package( OpenMP COMPONENTS C CXX Fortran )
endif()
set(HDF5_PREFER_PARALLEL true) # CMake sometimes mistakenly finds a serial system-provided HDF5.
find_package( HDF5 REQUIRED COMPONENTS C HL )
find_package( MPI REQUIRED )
find_package( jedicmake REQUIRED )
find_package( eckit 1.11.6 REQUIRED )
find_package( fckit 0.7.0 REQUIRED )
find_package( oops 1.0.0 REQUIRED )
find_package( Eigen3 REQUIRED NO_MODULE HINTS
              $ENV{Eigen3_ROOT} $ENV{EIGEN3_ROOT} $ENV{Eigen_ROOT} $ENV{EIGEN_ROOT}
              $ENV{Eigen3_PATH} $ENV{EIGEN3_PATH} $ENV{Eigen_PATH} $ENV{EIGEN_PATH} )
find_package( gsl-lite REQUIRED HINTS $ENV{gsl_lite_DIR} )
find_package( udunits 2.2.0 REQUIRED )

# Optional

### The ioda_data package is problematic in bundles because it has some special CMake logic
### at the root bundle location. So, we cannot find the ioda data while in ioda!
### Basically, with the current logic ioda_data must be loaded after ioda.
#find_package( ioda_data QUIET )   # Needed for tests

find_package( odc 1.0.2 QUIET )   # Needed for odc
find_package( Boost 1.64.0 )      # Provides an implementation of optional
find_package( Python3 COMPONENTS Interpreter Development )
find_package( pybind11 QUIET)

## Files used by both test and in non-test executables
install(DIRECTORY share/ioda
        TYPE DATA
        COMPONENT share )
set( IODA_YAML_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/share/ioda/yaml )

## Sources
add_subdirectory( src/engines )   # `ioda_engines` target
add_subdirectory( src )           # `ioda` target
add_subdirectory( src/mains )     # ioda executable targets
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

## Package Config
ecbuild_install_project( NAME ${PROJECT_NAME} )

ecbuild_print_summary()

