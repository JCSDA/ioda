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

set( ECBUILD_DEFAULT_BUILD_TYPE Release )
set( ENABLE_OS_TESTS           OFF CACHE BOOL "Disable OS tests" FORCE )
set( ENABLE_LARGE_FILE_SUPPORT OFF CACHE BOOL "Disable testing of large file support" FORCE )
set( ENABLE_MPI ON CACHE BOOL "Compile with MPI" )

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
set( Boost_MINIMUM_VERSION "1.47" )
find_package( Boost ${Boost_MINIMUM_VERSION} REQUIRED
              COMPONENTS unit_test_framework )
include_directories( ${Boost_INCLUDE_DIR} )

# NetCDF
set( NETCDF_F90 ON CACHE BOOL "Compile with Fortran NetCDF" )
find_package( NetCDF REQUIRED )
include_directories( ${NETCDF_INCLUDE_DIR} )

# LAPACK
ecbuild_add_option( FEATURE LAPACK
                    DESCRIPTION "Linear Algebra PACKage"
                    REQUIRED_PACKAGES "LAPACK REQUIRED" )

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

#find_package (ODB 1.0.3 COMPONENTS odb odbec odbemos odbifsaux)

#odb
ecbuild_add_option( FEATURE ODB
                    DEFAULT OFF
                    DESCRIPTION "Support for ODB"
                    REQUIRED_PACKAGES "PROJECT odb  VERSION 1.0.3" )

if (HAVE_ODB)
  set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -DHAVE_ODB")
  include_directories( ${ODB_INCLUDE_DIRS} )
endif (HAVE_ODB)

#odb api
ecbuild_add_option( FEATURE ODB_API
                    DEFAULT OFF
                    DESCRIPTION "Support for ODB API"
                    REQUIRED_PACKAGES "PROJECT odb_api  VERSION 0.17" )

if (HAVE_ODB_API)
  set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -DHAVE_ODB_API")
endif (HAVE_ODB_API)

################################################################################
# Export package info
################################################################################

list( APPEND IODA_TPLS NetCDF oops )

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

if( HAVE_ODB_API )
  list( APPEND IODA_EXTRA_LIBRARIES     Odb    )
endif()

if( HAVE_ODB )
  list( APPEND IODA_EXTRA_LIBRARIES     OOPS  ${ODB_LIBRARY_odb}     )
endif()

################################################################################
# Sources
################################################################################

include( ioda_compiler_flags )
include_directories( ${IODA_INCLUDE_DIRS} ${IODA_EXTRA_INCLUDE_DIRS} )
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

