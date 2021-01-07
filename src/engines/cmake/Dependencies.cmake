# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

set(HDF5_PREFER_PARALLEL true)
#set(HDF5_FIND_DEBUG true)
find_package(HDF5 REQUIRED COMPONENTS C HL)

if(HDF5_IS_PARALLEL)
	find_package(MPI REQUIRED)
endif()

#find_package(Eigen3 REQUIRED)
find_package( Eigen3 REQUIRED NO_MODULE HINTS
              $ENV{Eigen3_ROOT} $ENV{EIGEN3_ROOT} $ENV{Eigen_ROOT} $ENV{EIGEN_ROOT}
              $ENV{Eigen3_PATH} $ENV{EIGEN3_PATH} $ENV{Eigen_PATH} $ENV{EIGEN_PATH} )

find_package(gsl-lite REQUIRED HINTS $ENV{gsl_lite_DIR})

find_package(Python3 COMPONENTS Interpreter Development)

if (BUILD_PYTHON_BINDINGS)
	find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
	find_package(pybind11 REQUIRED)
endif()

find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --track-origins=yes --leak-check=full --show-leak-kinds=definite,indirect,possible" )
