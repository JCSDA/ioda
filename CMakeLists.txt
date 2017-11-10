cmake_minimum_required( VERSION 2.8.4 FATAL_ERROR )

project( ncdiag C Fortran )

set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} )

set( ECBUILD_DEFAULT_BUILD_TYPE Release )
set( ENABLE_OS_TESTS           OFF CACHE BOOL "Disable OS tests" FORCE )
set( ENABLE_LARGE_FILE_SUPPORT OFF CACHE BOOL "Disable testing of large file support" FORCE )

include( ecbuild_system NO_POLICY_SCOPE )

ecbuild_requires_macro_version( 2.5 )

ecbuild_declare_project()

ecbuild_enable_fortran( REQUIRED )

# NetCDF
set( NETCDF_F90 ON CACHE BOOL "Compile with Fortran NetCDF" )
find_package( NetCDF REQUIRED )
include_directories( ${NETCDF_INCLUDE_DIR} )

set( NCDIAG_LINKER_LANGUAGE Fortran )

include( ncdiag_compiler_flags )
add_subdirectory( src )

# print the summary of the configuration
ecbuild_print_summary()

