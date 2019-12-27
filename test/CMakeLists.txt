# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.yml
  testinput/iodatest_obsspace.yml
  testinput/iodatest_localobsspace.yml
  testinput/iodatest_obsvector.yml
  testinput/iodatest_fileio.yml
  testinput/iodatest_distribution.yml
  testinput/iodatest_obsspace_container.yml
  testinput/iodatest_obsspace_marine.yml
  testinput/iodatest_obsspace_fortran.yml
  testinput/iodatest_obsspace_localization.yml
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)
foreach(FILENAME ${ioda_test_input})
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} )
endforeach(FILENAME)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testoutput)

# Add files to cmake resources
ecbuild_add_resources( TARGET ioda_test_scripts
                       SOURCES_PACK
                       ${ioda_test_input}
                     )

# Create Data directory for test data and symlink files
list( APPEND ioda_test_data
      testinput_tier_1.tar.gz
      )

set(ECBUILD_DOWNLOAD_BASE_URL https://jedi-test-files.s3.amazonaws.com/ioda)

if (DEFINED ENV{LOCAL_PATH_TESTFILES})
	set(LOCAL_PATH_TESTFILES "$ENV{LOCAL_PATH_TESTFILES}")
endif()

if( NOT DEFINED LOCAL_PATH_TESTFILES )
  set(TESTFILE_DIR ${CMAKE_BINARY_DIR}/test_data/ioda CACHE PATH "data dir for test data")
  file(MAKE_DIRECTORY ${TESTFILE_DIR})
else()
  set(TESTFILE_DIR ${LOCAL_PATH_TESTFILES})
endif()

#check whether the URL exists or not 
ecbuild_check_multiurl(NAMES   ${ioda_test_data}
                       DIRHOST ${GIT_BRANCH}
                       RESULT  SPECIFIC_TEST_FILES)

# Set distant directory
if(${SPECIFIC_TEST_FILES} MATCHES 0)
    # Download and extract new test files (distant directory = git branch)
    set(DIRHOST ${GIT_BRANCH})
else()
    # Download and extract develop test files (distant directory = develop)
    set(DIRHOST "develop")
endif()
message(STATUS "Test data downloaded from: " ${ECBUILD_DOWNLOAD_BASE_URL}/${DIRHOST})



ecbuild_get_test_multidata ( TARGET get_ioda_test_data
                             NAMES ${ioda_test_data}
			     DIRHOST ${DIRHOST}
			     DIRLOCAL ${TESTFILE_DIR}
			     EXTRACT
                             )

execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
                 ${TESTFILE_DIR}
                 ${CMAKE_CURRENT_BINARY_DIR}/Data 
		 )
#####################################################################
# Add executables frequently used in tests

ecbuild_add_executable( TARGET  test_ObsSpace.x
                        SOURCES mains/TestObsSpace.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaObsSpace.x
                        SOURCES mains/TestIodaObsSpace.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_LocalObsSpace.x
                        SOURCES mains/TestLocalObsSpace.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaIO.x
                        SOURCES mains/TestIodaIO.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaDistribution.x
                        SOURCES mains/TestDistribution.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_ObsSpaceContainer.x
                        SOURCES mains/TestObsSpaceContainer.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_ObsVector.x
                        SOURCES mains/TestObsVector.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaObsVector.x
                        SOURCES mains/TestIodaObsVector.cc
                        LIBS    ioda
                       )

#####################################################################

ecbuild_add_test( TARGET  test_ioda_oops_obsvector
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsVector.x
                  ARGS    "testinput/iodatest_obsvector.yml"
                  DEPENDS test_ObsVector.x )

ecbuild_add_test( TARGET  test_ioda_obsvector
                  MPI     4
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsVector.x
                  ARGS    "testinput/iodatest_obsvector.yml"
                  DEPENDS test_IodaObsVector.x )

ecbuild_add_test( TARGET  test_ioda_distribution
                  MPI     4
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaDistribution.x
                  ARGS    "testinput/iodatest_distribution.yml"
                  DEPENDS test_IodaDistribution.x )

ecbuild_add_test( TARGET  test_ioda_obsspacecontainer
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsSpaceContainer.x
                  ARGS    "testinput/iodatest_obsspace_container.yml"
                  DEPENDS test_ObsSpaceContainer.x )

ecbuild_add_test( TARGET  test_ioda_oops_obsspace
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsSpace.x
                  ARGS    "testinput/iodatest.yml"
                  DEPENDS test_ObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_localobsspace
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_LocalObsSpace.x
                  ARGS    "testinput/iodatest_localobsspace.yml"
                  DEPENDS test_LocalObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_obsspace
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_obsspace.yml"
                  DEPENDS test_IodaObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_obsspace_mpi
                  MPI     2
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_obsspace.yml"
                  DEPENDS test_IodaObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_obsspace_marine
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_obsspace_marine.yml"
                  DEPENDS test_IodaObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_fileio
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaIO.x
                  ARGS    "testinput/iodatest_fileio.yml"
                  DEPENDS test_IodaIO.x )

ecbuild_add_test( TARGET  test_ioda_obsspace_localization
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_obsspace_localization.yml"
                  DEPENDS test_IodaObsSpace.x )

if( HAVE_FCTEST )
add_fctest( TARGET  test_ioda_obsspace_fortran
            SOURCES ioda/obsspace.F90
            ARGS    --config testinput/iodatest_obsspace_fortran.yml
            CONDITION HAVE_FCTEST
            LINKER_LANGUAGE Fortran
            LIBS    ioda )
endif()
