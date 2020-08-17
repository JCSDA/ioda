# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# function to remove a symlink if it exists
function( REMOVE_YAML_SYMLINK link )
    # if a leftover symlink is sitting around, remove it
    execute_process( COMMAND find ${link} -type l
                     OUTPUT_VARIABLE SYMLINK_FILE
                     OUTPUT_STRIP_TRAILING_WHITESPACE
                     ERROR_VARIABLE FIND_ERROR )
    if(SYMLINK_FILE)
      message( STATUS "Removing obsolete symlink: ${SYMLINK_FILE}" )
      execute_process( COMMAND rm -f ${SYMLINK_FILE} )
    endif()
endfunction()

# function to create targets to build test YAML
function( SET_TARGETS_DEPS_YAML caselist casemode depsvar )
  foreach(TEST_CASE ${caselist})
    set( SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/testinput/iodatest_${TEST_CASE}.yml" )
    if(${casemode} STREQUAL "ODC")
      set( DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/testinput/iodatest_${TEST_CASE}_odc.yml" )
      set( OBS_FILE_SUFFIX "odb" )
    else()
      set( DEST_FILE "${CMAKE_CURRENT_BINARY_DIR}/testinput/iodatest_${TEST_CASE}.yml" )
      set( OBS_FILE_SUFFIX "nc4" )
    endif()

    # if a leftover symlink is sitting around, remove it
    REMOVE_YAML_SYMLINK( ${DEST_FILE} )

    # add a target that runs a sed command to construct the yaml file
    add_custom_command( OUTPUT ${DEST_FILE}
      DEPENDS ${SOURCE_FILE}
      COMMAND sed -e's/@OBS_FILE_SUFFIX@/${OBS_FILE_SUFFIX}/g' ${SOURCE_FILE} > ${DEST_FILE} )
    list( APPEND test_case_deps ${DEST_FILE} )
  endforeach()
  set( ${depsvar} ${test_case_deps} PARENT_SCOPE )
endfunction()

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.yml
  testinput/iodatest_localobsspace.yml
  testinput/iodatest_obserror.yml
  testinput/iodatest_io.yml
  testinput/iodatest_io_odc.yml
  testinput/iodatest_distribution.yml
  testinput/iodatest_obsspace_container.yml
  testinput/iodatest_obsspace_marine.yml
  testinput/iodatest_obsspace_fortran.yml
  testinput/iodatest_obsspace_localization.yml
  testinput/iodatest_descendingsort.yml
)

list( APPEND ioda_repeat_test_cases
  obsspace
  obsvector
)

# Set up test input files (YAML configuration)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)

foreach(FILENAME ${ioda_test_input})
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} )
endforeach()

# contruct tests in ioda_repeat_test_cases for netcdf obs files
SET_TARGETS_DEPS_YAML( "${ioda_repeat_test_cases}" NETCDF ioda_test_nc_deps )

list( APPEND ioda_test_cases_deps ${ioda_test_nc_deps} ${ioda_test_odc_deps} )
add_custom_target( ioda_test_case_yaml ALL DEPENDS ${ioda_test_cases_deps} )

# Set up test output directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testoutput)

# Add files to cmake resources
ecbuild_add_resources( TARGET ioda_test_scripts
                       SOURCES_PACK ${ioda_test_input} )

# Create Data directory for test data and symlink files
list( APPEND ioda_test_data testinput_tier_1.tar.gz )

if( DEFINED ENV{LOCAL_PATH_TESTFILES_IODA})
    set(LOCAL_PATH_TESTFILES_IODA "$ENV{LOCAL_PATH_TESTFILES_IODA}")
endif()

# Set URL for IODA test files
set(IODA_DOWNLOAD_BASE_URL https://jedi-test-files.s3.amazonaws.com)

# If local path to testfiles is defined don't download
if( NOT DEFINED LOCAL_PATH_TESTFILES_IODA )

  message(STATUS "LOCAL_PATH_TESTFILES_IODA is not defined, download test files")
  if( NOT DEFINED IODA_IODA_TESTFILES_BRANCH)

    # Get the current git branch
    execute_process(
      COMMAND git rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_BRANCH_IODA
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "IODA branch [parsed]: ${GIT_BRANCH_IODA}")
  else()
    message(STATUS "IODA branch [user-set]: ${IODA_IODA_TESTFILES_BRANCH}")
    set(GIT_BRANCH_IODA ${IODA_IODA_TESTFILES_BRANCH})
  endif()
 
  # Check whether the URL exists or not 
  set(ECBUILD_DOWNLOAD_BASE_URL ${IODA_DOWNLOAD_BASE_URL}/ioda)

  ecbuild_check_urls(NAMES ${GIT_BRANCH_IODA}/${ioda_test_data}
                     RESULT IODA_SPECIFIC_TEST_FILES)

  # Set distant directory
  if( IODA_SPECIFIC_TEST_FILES MATCHES 0 )
    # Download and extract new test files (distant directory = git branch)
    set(DIRNAME ${GIT_BRANCH_IODA})
    message(STATUS "GIT_BRANCH_IODA is set. [download: ${DIRNAME}]")
  else()
    # Download and extract develop test files (distant directory = develop)
    set(DIRNAME "develop")
    message(STATUS "GIT_BRANCH_IODA not set. [download: ${DIRNAME}]")
  endif()
  message(STATUS "Test data will be downloaded from: ${IODA_DOWNLOAD_BASE_URL}/${DIRNAME}")

  set(TESTFILE_DIR_IODA ${CMAKE_BINARY_DIR}/test_data/ioda/${DIRNAME} CACHE PATH "data dir for test data")
  file(MAKE_DIRECTORY ${TESTFILE_DIR_IODA})
  message(STATUS "LOCAL_PATH_TESTFILES_IODA is not defined, download test files")

  # Run test to download data
  set(IODA_REP_NAME ioda)
  set(IODA_TESTFILES_NAME ${ioda_test_data})
  set(IODA_BRANCH_NAME ${DIRNAME})
  set(IODA_TESTFILES_PATH ${CMAKE_BINARY_DIR}/test_data/ioda/${DIRNAME})

  # Create download script for get_ioda_test_data test 
  set ( FILENAME ioda_data_downloader.py)
  set ( SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} )
  set ( DEST_FILE ${CMAKE_BINARY_DIR}/bin/${FILENAME} )
  list( APPEND bin_ioda_test_download_scripts_deps ${DEST_FILE} )

  if( EXISTS "${SOURCE_FILE}.in" )
    configure_file( ${SOURCE_FILE}.in ${DEST_FILE} @ONLY )
  else()
    configure_file( ${SOURCE_FILE}    ${DEST_FILE} @ONLY )
  endif()

  add_custom_target( bin_ioda_test_download_scripts ALL
    COMMAND chmod +x ${bin_ioda_test_download_scripts_deps}
    DEPENDS ${bin_ioda_test_download_scripts_deps} )


  ecbuild_add_test( TARGET    get_ioda_test_data
                    TYPE      SCRIPT
                    COMMAND    ${CMAKE_BINARY_DIR}/bin/ioda_data_downloader.py
                    ARGS      testoutput/download_test.log)
else()
  set(TESTFILE_DIR_IODA ${LOCAL_PATH_TESTFILES_IODA})
  message(STATUS "use LOCAL_PATH_TESTFILES_IODA: ${LOCAL_PATH_TESTFILES_IODA}")
endif()

execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
                 ${TESTFILE_DIR_IODA}
                 ${CMAKE_CURRENT_BINARY_DIR}/Data)

add_library(ioda_test INTERFACE)
set(BUILD_DIR_TEST_INCLUDE_PATH ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/test/include)
add_custom_target(ioda_test_headers ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_DIR_TEST_INCLUDE_PATH}/${PROJECT_NAME}"
                                    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR} ${BUILD_DIR_TEST_INCLUDE_PATH}/${PROJECT_NAME}/test)
add_dependencies(ioda_test ioda_test_headers)
target_include_directories(ioda_test INTERFACE $<BUILD_INTERFACE:${BUILD_DIR_TEST_INCLUDE_PATH}>)
target_link_libraries(ioda_test INTERFACE ioda)

# For the following test specifications:
#   All of the test data is downloaded by the above test "get_ioda_test_data".
#   Therefore, make "get_ioda_test_data" a dependency (TEST_DEPENDS) for all
#   of the following tests.
#
#   The first test in each group specifies a target to be compiled from source
#   code. Subsequent tests (in each group) use that compiled target for their tests.
#   Therefore, the first test in each group uses ecbuild(TARGET SOURCES ...)
#   specification, and the subsequent tests use ecbuild(TARGET COMMAND ...)"
#   specification.

#####################################################################
# IodaIO tests
#####################################################################
ecbuild_add_test( TARGET  test_ioda_io
                  SOURCES mains/TestIodaIO.cc
                  ARGS    "testinput/iodatest_io.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

if( odc_FOUND )
ecbuild_add_test( TARGET  test_ioda_io_odc
                  COMMAND test_ioda_io
                  ARGS    "testinput/iodatest_io_odc.yml"
                  TEST_DEPENDS get_ioda_test_data )
endif()

#####################################################################
# Distribution tests
#####################################################################
ecbuild_add_test( TARGET  test_ioda_distribution
                  MPI     4
                  SOURCES mains/TestDistribution.cc
                  ARGS    "testinput/iodatest_distribution.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsSpaceContainer tests
#####################################################################
ecbuild_add_test( TARGET  test_ioda_obsspacecontainer
                  SOURCES mains/TestObsSpaceContainer.cc
                  ARGS    "testinput/iodatest_obsspace_container.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsSpace tests
#####################################################################

# OOPS ObsSpace interface
ecbuild_add_test( TARGET  test_ioda_oops_obsspace
                  SOURCES mains/TestObsSpace.cc
                  ARGS    "testinput/iodatest.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA ObsSpace class
ecbuild_add_test( TARGET  test_ioda_obsspace
                  SOURCES mains/TestIodaObsSpace.cc
                  ARGS    "testinput/iodatest_obsspace.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_mpi
                  MPI     2
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace.yml"
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_marine
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_marine.yml"
                  TEST_DEPENDS get_ioda_test_data )

# IODA ObsSpace class - Fortran interface test
if( HAVE_FCTEST )
add_fctest( TARGET  test_ioda_obsspace_fortran
            SOURCES ioda/obsspace.F90
            ARGS    --config testinput/iodatest_obsspace_fortran.yml
            CONDITION HAVE_FCTEST
            LINKER_LANGUAGE Fortran
            LIBS  ioda_test
            TEST_DEPENDS get_ioda_test_data )
endif()

# OOPS LocalObsSpace test
ecbuild_add_test( TARGET  test_ioda_localobsspace
                  SOURCES mains/TestLocalObsSpace.cc
                  ARGS    "testinput/iodatest_localobsspace.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA LocalObsSpace test
ecbuild_add_test( TARGET  test_ioda_obsspace_localization
                  SOURCES mains/TestIodaLocalObsSpace.cc
                  ARGS    "testinput/iodatest_obsspace_localization.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA ObsSpace sorting of obs groups
ecbuild_add_test( TARGET  test_ioda_descendingsort
                  SOURCES mains/TestDescendingSort.cc
                  ARGS    "testinput/iodatest_descendingsort.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsVector tests
#####################################################################

# OOPS ObsVector interface
ecbuild_add_test( TARGET  test_ioda_oops_obsvector
                  SOURCES mains/TestObsVector.cc
                  ARGS    "testinput/iodatest_obsvector.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA ObsVector class
ecbuild_add_test( TARGET  test_ioda_obsvector
                  MPI     4
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS    "testinput/iodatest_obsvector.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsErrorCovariance tests
#####################################################################

# OOPS ObsErrorCovariance interface (dependent on ObsVector)
ecbuild_add_test( TARGET  test_ioda_oops_obserrorcovariance
                  SOURCES mains/TestObsErrorCovariance.cc
                  ARGS    "testinput/iodatest_obserror.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# Test the time_IodaIO.x app
#####################################################################

ecbuild_add_test( TARGET  test_ioda_time_io
                  COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                  ARGS    "testinput/iodatest_obsspace.yml"
                  TEST_DEPENDS get_ioda_test_data )
