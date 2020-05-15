# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

################################################################################
# IODA
################################################################################

cmake_minimum_required( VERSION 3.3.2 FATAL_ERROR )

project( ioda C CXX Fortran )

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH} )

set( CMAKE_DIRECTORY_LABELS "ioda" )

set( ECBUILD_DEFAULT_BUILD_TYPE Release )
set( ENABLE_OS_TESTS           OFF CACHE BOOL "Disable OS tests" FORCE )
set( ENABLE_LARGE_FILE_SUPPORT OFF CACHE BOOL "Disable testing of large file support" FORCE )

include( ecbuild_system NO_POLICY_SCOPE )

ecbuild_requires_macro_version( 2.7 )

################################################################################
# Project
################################################################################

ecbuild_declare_project()

ecbuild_enable_fortran( REQUIRED )
ecbuild_add_cxx11_flags()

set( IODA_LINKER_LANGUAGE CXX )

################################################################################
# Dependencies
################################################################################

# Boost
include_directories( ${Boost_INCLUDE_DIR} )

# NetCDF
set( NETCDF_F90 ON CACHE BOOL "Compile with Fortran NetCDF" )
find_package( NetCDF REQUIRED )
include_directories( ${NETCDF_INCLUDE_DIR} )

# eckit
ecbuild_use_package( PROJECT eckit VERSION 0.18.0 REQUIRED )
include_directories( ${ECKIT_INCLUDE_DIRS} )

# fckit
ecbuild_use_package( PROJECT fckit VERSION 0.4.1 REQUIRED )
include_directories( ${FCKIT_INCLUDE_DIRS} )

# oops
ecbuild_use_package( PROJECT oops VERSION 0.2.1 REQUIRED )
include_directories( ${OOPS_INCLUDE_DIRS} )
link_directories( ${oops_BASE_DIR}/lib )

# enable fctest
ecbuild_add_option( FEATURE FCTEST  DEFAULT ${ENABLE_TESTS}
                    DESCRIPTION "Fortran Unit Testing Framework"
                    REQUIRED_PACKAGES "PROJECT fckit" )

################################################################################
# Export package info
################################################################################

list( APPEND IODA_TPLS NetCDF oops ioda-engines )

set( IODA_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src
                       ${CMAKE_Fortran_MODULE_DIRECTORY} )
set( IODA_LIBRARIES ioda )

get_directory_property( IODA_DEFINITIONS COMPILE_DEFINITIONS )

foreach( _tpl ${IODA_TPLS} )
  string( TOUPPER ${_tpl} TPL )
  list( APPEND IODA_EXTRA_DEFINITIONS   ${${TPL}_DEFINITIONS}  ${${TPL}_TPL_DEFINITIONS}  )
  list( APPEND IODA_EXTRA_INCLUDE_DIRS  ${${TPL}_INCLUDE_DIRS} ${${TPL}_TPL_INCLUDE_DIRS} )
  list( APPEND IODA_EXTRA_LIBRARIES     ${${TPL}_LIBRARIES}    ${${TPL}_TPL_LIBRARIES}    )
endforeach()

################################################################################
# Sources
################################################################################

include( ioda_compiler_flags )
include_directories( ${IODA_INCLUDE_DIRS} ${IODA_EXTRA_INCLUDE_DIRS} )
include( ioda_extra_macros )
add_subdirectory( src )
add_subdirectory( test )
add_subdirectory( tools )

if(ECBUILD_INSTALL_FORTRAN_MODULES)
  install(DIRECTORY ${CMAKE_Fortran_MODULE_DIRECTORY}/${CMAKE_CFG_INTDIR} DESTINATION ${INSTALL_INCLUDE_DIR} )
endif()

################################################################################
# Finalise configuration
################################################################################

# prepares a tar.gz of the sources and/or binaries
ecbuild_install_project( NAME ioda )

# print the summary of the configuration
ecbuild_print_summary()

