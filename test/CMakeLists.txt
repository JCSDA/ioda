# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.yml
  testinput/iodatest_localobsspace.yml
  testinput/iodatest_obserror.yml
  testinput/iodatest_io.yml
  testinput/iodatest_distribution.yml
  testinput/iodatest_obsspace_container.yml
  testinput/iodatest_obsspace.yml
  testinput/iodatest_obsspace_marine.yml
  testinput/iodatest_obsspace_fortran.yml
  testinput/iodatest_obsspace_localization.yml
  testinput/iodatest_obsvector.yml
  testinput/iodatest_descendingsort.yml
)

# Set up test input files (YAML configuration)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)

foreach(FILENAME ${ioda_test_input})
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} )
endforeach()

# Set up test output directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testoutput)

# Add files to cmake resources
ecbuild_add_resources( TARGET ioda_test_scripts
                       SOURCES_PACK ${ioda_test_input} )

# Create Data directory for test data and symlink files
# Set list of test file tar balls
list( APPEND ioda_test_data ioda_testinput_tier_1.tar.gz )

# Check if user wants to use locally downloaded test data
if( DEFINED ENV{LOCAL_PATH_JEDI_TESTFILES})
    set(LOCAL_PATH_JEDI_TESTFILES "$ENV{LOCAL_PATH_JEDI_TESTFILES}")
endif()

# Determine the branch name
# Deafult is to use the name corresponding to the currently checked out branch or tag
# User can override through the IODA_TESTFILES_BRANCH environment variable
set( IODA_TESTFILES_BRANCH "$ENV{IODA_TESTFILES_BRANCH}")
if ( IODA_TESTFILES_BRANCH STREQUAL "" )
  # Get the current git branch
  execute_process(
    COMMAND bash "-c" "git symbolic-ref -q --short HEAD || git describe --tags --exact-match"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH_IODA
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "IODA branch [parsed]: ${GIT_BRANCH_IODA}")
else()
  message(STATUS "IODA branch [user-set]: ${IODA_TESTFILES_BRANCH}")
  set(GIT_BRANCH_IODA ${IODA_TESTFILES_BRANCH})
endif()

# Set up the download mechanism if local test files are not being used
if( NOT DEFINED LOCAL_PATH_JEDI_TESTFILES )
  message(STATUS "LOCAL_PATH_JEDI_TESTFILES is not defined, download test files")
 
  # Set URL for IODA test files
  if (${GIT_BRANCH_IODA} STREQUAL "1.0.0")
    # Release 1.0.0 data comes from UCAR hosting
    set(ECBUILD_DOWNLOAD_BASE_URL https://dashrepo.ucar.edu/dataset/147_miesch/file/ioda)
    set(DIRNAME ${GIT_BRANCH_IODA})
  else ()
    # All other data comes from AWS hosting
    # Using the ecbuild_check_urls to see if the data is on AWS in a bucket
    # corresponding to the checked out branch. The ecbuild_check_urls uses the variable
    # ECBUILD_DOWNLOAD_BASE_URL which must be set to a path that is to be checked for
    # the existence of files. The NAMES argument to ecbuild_check_urls specifies paths
    # to files underneath ECBUILD_DOWNLOAD_BASE_URL that are to be checked for existence.
    # The number of files NOT found is returned in the variable specified by RESULT.
    set(ECBUILD_DOWNLOAD_BASE_URL https://jedi-test-files.s3.amazonaws.com/ioda)
    ecbuild_check_urls(NAMES ${GIT_BRANCH_IODA}/${ioda_test_data}
                       RESULT IODA_SPECIFIC_TEST_FILES)

    # Set distant directory
    if( IODA_SPECIFIC_TEST_FILES MATCHES 0 )
      # The files in GIT_BRANCH_IODA do indeed exist, so use them
      # Download and extract new test files (distant directory = git branch)
      set(DIRNAME ${GIT_BRANCH_IODA})
      message(STATUS "GIT_BRANCH_IODA is set. [download: ${DIRNAME}]")
    else()
      # The files in GIT_BRANCH_IODA do not exist, fall back on develop
      # Download and extract develop test files (distant directory = develop)
      set(DIRNAME "develop")
      message(STATUS "GIT_BRANCH_IODA not set. [download: ${DIRNAME}]")
    endif()
  # Set another variable with the full path to the download files that were found.
  set(IODA_DOWNLOAD_BASE_URL "${ECBUILD_DOWNLOAD_BASE_URL}/${DIRNAME}")
  message(STATUS "Test data will be downloaded from: ${IODA_DOWNLOAD_BASE_URL}")
  endif()

  set(TESTFILE_DIR_IODA "${CMAKE_BINARY_DIR}/test_data/ioda/${DIRNAME}")
  file(MAKE_DIRECTORY ${TESTFILE_DIR_IODA})

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
                    COMMAND   ${CMAKE_BINARY_DIR}/bin/ioda_data_downloader.py
                    ARGS      ${IODA_DOWNLOAD_BASE_URL} ${TESTFILE_DIR_IODA} ${ioda_test_data})
else()
  # use local test files
  set(TESTFILE_DIR_IODA "${LOCAL_PATH_JEDI_TESTFILES}/ioda/${GIT_BRANCH_IODA}")
  message(STATUS "use LOCAL_PATH_JEDI_TESTFILES: ${LOCAL_PATH_JEDI_TESTFILES}")
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
