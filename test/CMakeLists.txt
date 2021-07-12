# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include( git_functions )

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.yml
  testinput/iodatest_obserror.yml
  testinput/iodatest_obsio_constructor.yml
  testinput/iodatest_obsio_read.yml
  testinput/iodatest_obsio_write.yml
  testinput/iodatest_obsframe_constructor.yml
  testinput/iodatest_obsframe_read.yml
  testinput/iodatest_obsframe_write.yml
  testinput/iodatest_distribution.yml
  testinput/iodatest_distribution_masterandreplica_mpi_2.yml
  testinput/iodatest_distribution_masterandreplica_mpi_3.yml
  testinput/iodatest_distribution_masterandreplica_mpi_4.yml
  testinput/iodatest_distribution_methods.yml
  testinput/iodatest_distribution_timewindow.yml
  testinput/iodatest_obsdatavector.yml
  testinput/iodatest_obsdtype.yml
  testinput/iodatest_obsspace.yml
  testinput/iodatest_obsspace_out_dims_check.yml
  testinput/iodatest_obsspace_grouping.yml
  testinput/iodatest_obsspace_index_recnum.yml
  testinput/iodatest_obsspace_index_recnum_twfilt.yml
  testinput/iodatest_obsspace_invalid_numeric.yml
  testinput/iodatest_obsspace_index_recnum_read_distributed_files.yml
  testinput/iodatest_obsspace_marine.yml
  testinput/iodatest_obsspace_mpi.yml
  testinput/iodatest_obsspace_fortran.yml
  testinput/iodatest_obsspace_put_db_channels.yml
  testinput/iodatest_obsvector.yml
  testinput/iodatest_obsvector_packeigen.yml
  testinput/iodatest_obsvector_pio_observer.yml
  testinput/iodatest_obsvector_pio_read_distributed_files.yml
  testinput/iodatest_descendingsort.yml
  testinput/iodatest_extendedobsspace.yml
  testinput/iodatest_extendedobsspace_halo.yml
  testinput/iodatest_odbqueryparams.yml
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

###############################
# Download/link ioda test files
###############################

# If IODA branch is being built set GIT_BRANCH_FUNC to IODA's current branch.
# If a tagged version of IODA is being built set GIT_TAG_FUNC to ioda's current tag.
find_branch_name(REPO_DIR_NAME ioda)

if( DEFINED GIT_BRANCH_FUNC )
  set( IODA_GIT_BRANCH ${GIT_BRANCH_FUNC} )
elseif( DEFINED GIT_TAG_FUNC )
  set( IODA_GIT_BRANCH ${GIT_TAG_FUNC} )
endif()

message ( STATUS "IODA_GIT_BRANCH: ${IODA_GIT_BRANCH}" )
if( DEFINED ENV{LOCAL_PATH_JEDI_TESTFILES} )
  # When env veriable LOCAL_PATH_JEDI_TESTFILES is set simply link test files
  # to build directory. get_ioda_test_data checks the existence of test file directory.
  # ioda test data must be stored in
  # ${LOCAL_PATH_JEDI_TESTFILES}/ioda/${BRANCH}/testinput_tier_1

  set( LOCAL_PATH_JEDI_TESTFILES "$ENV{LOCAL_PATH_JEDI_TESTFILES}" )
  message( STATUS "use LOCAL_PATH_JEDI_TESTFILES: ${LOCAL_PATH_JEDI_TESTFILES}" )

  # If test data specific to testing branch exists locally use it.
  # If not use test data specific to develop branch.
  if( EXISTS ${LOCAL_PATH_JEDI_TESTFILES}/ioda/${IODA_GIT_BRANCH} )
    set( TESTFILE_DIR_IODA "${LOCAL_PATH_JEDI_TESTFILES}/ioda/${IODA_GIT_BRANCH}" )
  else()
    set( TESTFILE_DIR_IODA "${LOCAL_PATH_JEDI_TESTFILES}/ioda/develop" )
  endif()

  message( STATUS "Test data in ${TESTFILE_DIR_IODA} is linked to build directory" )
  list( APPEND IODA_DATA_DOWNLOADER_ARGS
      ${TESTFILE_DIR_IODA} )
  set ( IODA_DATA_DOWNLOADER ioda_data_checker.py )

elseif( DEFINED GIT_TAG_FUNC )
  # Any tagged version of IODA is being built.
  # Build get_ioda_test_data test to download test data from DASH.
  message( STATUS "Tagged version of IODA is used" )

  # set ARGS for get_ioda_test_data
  set( IODA_DOWNLOAD_BASE_URL https://dashrepo.ucar.edu/api/v1/dataset/147_miesch/version/1.1.0/file )
  set( DIRNAME ${GIT_TAG_FUNC} )
  set( checksum "0" )
  set( TESTFILE_DIR_IODA "${CMAKE_SOURCE_DIR}/test-data-release/ioda/${DIRNAME}" )

  list( APPEND IODA_DATA_DOWNLOADER_ARGS
        ${IODA_DOWNLOAD_BASE_URL}
        ${CMAKE_SOURCE_DIR}/test-data-release
        ${ioda_test_data}
        ${GIT_TAG_FUNC}
        ioda)
  message( STATUS "Test data will be downloaded from: ${IODA_DOWNLOAD_BASE_URL}" )

  # Create test-data-release in source directory
  file( MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/test-data-release )

  set ( IODA_DATA_DOWNLOADER ioda_dash_downloader.py.in )
else()
  # Any branch of IODA is being built.
  # ioda-data repository is already cloned by bundle/CMakeLists.txt.
  # Link test files in ioda-data repo to build directory.
  # get_ioda_test_data checks the existence of test file directory.
  set( TESTFILE_DIR_IODA ${CMAKE_SOURCE_DIR}/ioda-data )
  list( APPEND IODA_DATA_DOWNLOADER_ARGS
      ${TESTFILE_DIR_IODA} )
  set (IODA_DATA_DOWNLOADER ioda_data_checker.py )
endif()

# Create download script for get_ioda_test_data test
set ( FILENAME ${IODA_DATA_DOWNLOADER})
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
                  COMMAND   ${CMAKE_BINARY_DIR}/bin/${IODA_DATA_DOWNLOADER}
                  ARGS  ${IODA_DATA_DOWNLOADER_ARGS} )

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

#####################################################################
# IO tests
#####################################################################
ecbuild_add_test( TARGET  test_ioda_obsio_constructor
                  SOURCES mains/TestObsIoConstructor.cc
                  ARGS    "testinput/iodatest_obsio_constructor.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsio_read
                  SOURCES mains/TestObsIoRead.cc
                  ARGS    "testinput/iodatest_obsio_read.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsio_write
                  SOURCES mains/TestObsIoWrite.cc
                  ARGS    "testinput/iodatest_obsio_write.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsframe_constructor
                  SOURCES mains/TestObsFrameConstructor.cc
                  ARGS    "testinput/iodatest_obsframe_constructor.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsframe_read
                  SOURCES mains/TestObsFrameRead.cc
                  ARGS    "testinput/iodatest_obsframe_read.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsframe_write
                  SOURCES mains/TestObsFrameWrite.cc
                  ARGS    "testinput/iodatest_obsframe_write.yml"
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

ecbuild_add_test( TARGET  test_ioda_distribution_masterandreplica_mpi_2
                  MPI     2
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_masterandreplica_mpi_2.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_distribution_masterandreplica_mpi_3
                  MPI     3
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_masterandreplica_mpi_3.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_distribution_masterandreplica_mpi_4
                  MPI     4
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_masterandreplica_mpi_4.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_distribution_timewindow
                  MPI     4
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_timewindow.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# Disrtibution method tests
ecbuild_add_test( TARGET  test_ioda_distribution_methods
                  MPI     4
                  SOURCES mains/TestDistributionMethods.cc
                  ARGS    "testinput/iodatest_distribution_methods.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )
#####################################################################
# Parameters tests
#####################################################################
ecbuild_add_test( TARGET  test_ioda_odbquery_parameters
                  SOURCES mains/TestOdbQueryParameters.cc
                  ARGS    "testinput/iodatest_odbqueryparams.yml"
                  LIBS    ioda_test )
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

ecbuild_add_test( TARGET  test_ioda_obsspace_grouping
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_grouping.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum
                  SOURCES mains/TestIodaObsSpaceIndexRecnum.cc
                  ARGS    "testinput/iodatest_obsspace_index_recnum.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_mpi_2
                  MPI 2
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_mpi_4
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_mpi_2
                  MPI 2
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_mpi_4
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_invalid_numeric
                  SOURCES mains/TestObsSpaceInvalidNumeric.cc
                  ARGS    "testinput/iodatest_obsspace_invalid_numeric.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_read_distributed_files
                  MPI 3
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_read_distributed_files.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_mpi
                  MPI     2
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_mpi.yml"
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_marine
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_marine.yml"
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_put_db_channels
                  SOURCES mains/TestIodaObsSpacePutDbChannels.cc
                  ARGS    "testinput/iodatest_obsspace_put_db_channels.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_out_dims_check
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    hdf5
                          "${CMAKE_CURRENT_BINARY_DIR}/test_ioda_oops_obsspace
                          testinput/iodatest_obsspace_out_dims_check.yml"
                          out_dims_check_0000.nc4
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

# IODA ObsSpace sorting of obs groups
ecbuild_add_test( TARGET  test_ioda_descendingsort
                  SOURCES mains/TestDescendingSort.cc
                  ARGS    "testinput/iodatest_descendingsort.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA extended ObsSpace
ecbuild_add_test( TARGET  test_ioda_extendedobsspace
                  SOURCES mains/TestExtendedObsSpace.cc
                  ARGS    "testinput/iodatest_extendedobsspace.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_MPI_2
                  MPI     2
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_MPI_3
                  MPI     3
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_halo_MPI_2
                  MPI     2
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace_halo.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_halo_MPI_3
                  MPI     3
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace_halo.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsVector tests
#####################################################################

# OOPS ObsVector interface
ecbuild_add_test( TARGET  test_ioda_oops_obsvector
                  MPI     4
                  SOURCES mains/TestObsVector.cc
                  ARGS    "testinput/iodatest_obsvector.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_oops_obsdatavector
                  SOURCES mains/TestObsDataVector.cc
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

# IODA ObsVector class (packEigen method)
ecbuild_add_test( TARGET  test_ioda_obsvector_packeigen
                  MPI     4
                  SOURCES mains/TestIodaObsVectorPackEigen.cc
                  ARGS    "testinput/iodatest_obsvector_packeigen.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA ObsVector class (Halo) parallel IO Observer
ecbuild_add_test( TARGET  test_ioda_obsvector_pio_observer
                  MPI     4
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS    "testinput/iodatest_obsvector_pio_observer.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA ObsVector class (Halo) parallel IO read from distributed files.
ecbuild_add_test( TARGET  test_ioda_obsvector_pio_read_distributed_files
                  MPI     4
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS    "testinput/iodatest_obsvector_pio_read_distributed_files.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS test_ioda_obsvector_pio_observer )

#####################################################################
# ObsDataVector tests
#####################################################################

# IODA ObsDataVector class
ecbuild_add_test( TARGET  test_ioda_obsdatavector
                  MPI     2
                  SOURCES mains/TestIodaObsDataVector.cc
                  ARGS    "testinput/iodatest_obsdatavector.yml"
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


#####################################################################
# Test the ioda-upgrade.x app
#####################################################################

ecbuild_add_test( TARGET  test_ioda_upgrader_amsua_n19
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_amsua_n19.nc4
                          testoutput/upgrader_amsua_n19.nc4"
                          upgrader_amsua_n19.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_gmi_gpm
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_gmi_gpm.nc4
                          testoutput/upgrader_gmi_gpm.nc4"
                          upgrader_gmi_gpm.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_mhs_metop-b
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_mhs_metop-b.nc4
                          testoutput/upgrader_mhs_metop-b.nc4"
                          upgrader_mhs_metop-b.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_sondes
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_sondes.nc4
                          testoutput/upgrader_sondes.nc4"
                          upgrader_sondes.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_aod
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_aod.nc4
                          testoutput/upgrader_aod.nc4"
                          upgrader_aod.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_gnssro
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_gnssro.nc4
                          testoutput/upgrader_gnssro.nc4"
                          upgrader_gnssro.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_amsua_n19_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: amsua_n19 netcdf compatibilty check"
                          upgrader_amsua_n19.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_amsua_n19 )

ecbuild_add_test( TARGET  test_ioda_upgrader_gmi_gpm_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: gmi_gpm netcdf compatibilty check"
                          upgrader_gmi_gpm.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_gmi_gpm )

ecbuild_add_test( TARGET  test_ioda_upgrader_mhs_metop-b_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: mhs_metop-b netcdf compatibilty check"
                          upgrader_mhs_metop-b.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_mhs_metop-b )

ecbuild_add_test( TARGET  test_ioda_upgrader_sondes_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: sondes netcdf compatibilty check"
                          upgrader_sondes.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_sondes )

ecbuild_add_test( TARGET  test_ioda_upgrader_aod_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: aod netcdf compatibilty check"
                          upgrader_aod.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_aod )

ecbuild_add_test( TARGET  test_ioda_upgrader_gnssro_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: gnssro netcdf compatibilty check"
                          upgrader_gnssro.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_gnssro )

#####################################################################
# ObsDtype tests
#####################################################################

ecbuild_add_test( TARGET  test_ioda_obsdtype
                  SOURCES mains/TestIodaObsDtype.cc
                  ARGS    "testinput/iodatest_obsdtype.yml"
                  LIBS    ioda_test )
