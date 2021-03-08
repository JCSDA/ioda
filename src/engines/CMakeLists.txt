# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

cmake_minimum_required(VERSION 3.12)

set( CMAKE_DIRECTORY_LABELS "ioda-engines" )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Record whether this is the top-level project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(IODA_ENGINES_IS_PROJECT True)
else()
	set(IODA_ENGINES_IS_PROJECT False)
endif()

set(IODA_ENGINES_MAJOR 0 CACHE INTERNAL "ioda engines major version" FORCE)
set(IODA_ENGINES_MINOR 9 CACHE INTERNAL "ioda engines minor version" FORCE)
set(IODA_ENGINES_REVISION 0 CACHE INTERNAL "ioda engines revision version" FORCE)
set(IODA_ENGINES_TWEAK 0 CACHE INTERNAL "ioda engines tweak version" FORCE)
set(IODA_ENGINES_VERSION ${IODA_ENGINES_MAJOR}.${IODA_ENGINES_MINOR}.${IODA_ENGINES_REVISION}.${IODA_ENGINES_TWEAK}
	CACHE INTERNAL "ioda engines version" FORCE)

if(IODA_ENGINES_IS_PROJECT)
	project(ioda_engines VERSION ${IODA_ENGINES_VERSION} LANGUAGES C CXX)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
		set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release Debug RelWithDebInfo MinSizeRel)
	endif()
endif()

option(BUILD_PYTHON_BINDINGS "Build Python bindings using pybind11?" OFF)

include(GNUInstallDirs)

include(Dependencies)

if(IODA_ENGINES_IS_PROJECT)
	# Note: Dependencies come first for HDF5 version tagging.
	set(CTEST_USE_LAUNCHERS ON)
	include(cmake/cdash-integration.cmake)
	include(CTest)
	include(CTestUseLaunchers)

	include(Docs)

	# Installation instructions that must be set before defining targets.
	include(Installing)
endif()

add_subdirectory(ioda)
add_subdirectory(test)
add_subdirectory(Examples)

if(IODA_ENGINES_IS_PROJECT)
	# Packaging instructions
	include(Packaging)
endif()

