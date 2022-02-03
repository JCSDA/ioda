# (C) Copyright 2020-2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.


# Required packages

## jedi-cmake provides some find_package scripts that we need
find_package( jedicmake REQUIRED )


## gsl-lite is an implementation of the guidelines support library.
## We use this for gsl::span and gsl::narrow.
## std::span is a feature of C++20, and we can check for presence / disable this package
##   in the future.
## gsl::narrow can be re-implemented locally. It performs a checked narrowing conversion,
##   and throws if the value changes. Think int -> unsigned int with a negative number.
find_package(gsl-lite REQUIRED HINTS $ENV{gsl_lite_DIR})

## HDF5 could be optional, but then the library couldn't do much. This may change if we
## ever write a NetCDF engine, or if the ODB engine becomes much more mature.
set(HDF5_PREFER_PARALLEL true)
find_package(HDF5 REQUIRED COMPONENTS C HL)
if(HDF5_IS_PARALLEL)
	find_package(MPI REQUIRED)
endif()

## Eigen is used as one possiblity for multidimensional data transfer.
## We could also use xtensor or blaze.
## For the future, perhaps these Eigen bindings can be made optional. We will always
##   use Eigen in oops and beyond, but it simplifies the build of Python modules.
find_package( Eigen3 REQUIRED NO_MODULE HINTS
              $ENV{Eigen3_ROOT} $ENV{EIGEN3_ROOT} $ENV{Eigen_ROOT} $ENV{EIGEN_ROOT}
              $ENV{Eigen3_PATH} $ENV{EIGEN3_PATH} $ENV{Eigen_PATH} $ENV{EIGEN_PATH} )

find_package( udunits 2.2.0 REQUIRED )  # Needed for unit conversions

# Optional packages

## We need Python3 when constructing Python bindings, of course.
## pybind11 occasionally picks the wrong Python version, and you may need to set
## other CMake options to find the right version.
if (BUILD_PYTHON_BINDINGS)
	find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
	find_package(pybind11 REQUIRED)
endif()

# This sets appropriate options for "ctest -D ExperimentalMemCheck".
find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --track-origins=yes --leak-check=full --show-leak-kinds=definite,indirect,possible" )

find_package(ecbuild 3.3.2 QUIET)
if (ecbuild_FOUND)
	include( ecbuild_system NO_POLICY_SCOPE )
endif()

# eckit is needed for odb tests
find_package(eckit 1.11.6 QUIET)

# odc is needed to enable the ODC engine
find_package(odc 1.0.2 QUIET)

# Boost provides an implementation of optional. Should be removed.
find_package(Boost 1.64.0 QUIET)

# oops
# Currently disabled / provided by the top-level ioda CMakeLists.txt file.
# The issue is that OpenMP's Fortran interface cannot always be found even when present.
#find_package( OpenMP COMPONENTS CXX Fortran )
# oops is used for YAML validation
#find_package(oops 1.0.0 QUIET)

