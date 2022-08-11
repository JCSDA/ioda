# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Includes
include( git_functions )

# Function definitions

if (odc_FOUND)
  find_package( NetCDF REQUIRED ) # need this for the ncdump executable

  set(ODC_TOOL_PATH "${odc_BASE_DIR}/bin/odc")

  if (NOT ${Python3_FOUND})
    message(WARNING "Python3 executable not found. Some ODC tests will be disabled.")
  endif()

  function( add_test_checking_odb_to_netcdf_conversion_results_against_odb_sql_queries )
    set( options )
    set( single_value_args TEST_NAME )
    set( multi_value_args  ODB_FILE )

    cmake_parse_arguments( _PAR "${options}" "${single_value_args}" "${multi_value_args}"
                           ${_FIRST_ARG} ${ARGN} )
    if(_PAR_UNPARSED_ARGUMENTS)
      ecbuild_critical("Unknown keywords given to add_test_comparing_netcdf_variables_with_odb_sql_queries(): \"${_PAR_UNPARSED_ARGUMENTS}\"")
    endif()

    if (${Python3_FOUND})
      ecbuild_add_test( TARGET  test_ioda-convert_${_PAR_TEST_NAME}_odc
                        COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                        ARGS    convert_${_PAR_TEST_NAME}
                                Data/testinput_tier_1/${_PAR_ODB_FILE}
                                testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_name_map.yml
                                testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_query.yml
                        TEST_DEPENDS get_ioda_test_data )
      set_tests_properties( test_ioda-convert_${_PAR_TEST_NAME}_odc
                            PROPERTIES FIXTURES_SETUP ioda-convert_${_PAR_TEST_NAME}_odc )
      ecbuild_add_test( TARGET  test_ioda-convert_${_PAR_TEST_NAME}_odc_compare
                        COMMAND ${Python3_EXECUTABLE}
                        ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare_odc_with_netcdf.py
                                --odc ${ODC_TOOL_PATH}
                                testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_comparison.conf
                                Data/testinput_tier_1/${_PAR_ODB_FILE}
                                testoutput/test-convert_${_PAR_TEST_NAME}.hdf
                        TEST_DEPENDS get_ioda_test_data )
      set_tests_properties( test_ioda-convert_${_PAR_TEST_NAME}_odc_compare
                            PROPERTIES FIXTURES_REQUIRED ioda-convert_${_PAR_TEST_NAME}_odc )
    endif()

    list( APPEND ioda_test_input
      testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_name_map.yml
      testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_query.yml
      testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_comparison.conf )
    set( ioda_test_input ${ioda_test_input} PARENT_SCOPE )
  endfunction()
endif()

# The list of all test input config files, which need to be symlinked from the build folder.
# (These symlinks are created at the end of this file, to give CMake functions setting up certain
# tests a chance to append items to this list).
list( APPEND ioda_test_input
  testinput/iodatest.yml
  testinput/iodatest_fileformat.yml
  testinput/iodatest_remove_hdf5_variable.yml
  testinput/iodatest_copy_attributes.yml
  testinput/iodatest_obserror.yml
  testinput/iodatest_obsio_constructor.yml
  testinput/iodatest_obsio_read.yml
  testinput/iodatest_obsio_write.yml
  testinput/iodatest_obsframe_constructor.yml
  testinput/iodatest_obsframe_read.yml
  testinput/iodatest_distribution.yml
  testinput/iodatest_distribution_masterandreplica_mpi_2.yml
  testinput/iodatest_distribution_masterandreplica_mpi_3.yml
  testinput/iodatest_distribution_masterandreplica_mpi_4.yml
  testinput/iodatest_distribution_methods.yml
  testinput/iodatest_distribution_timewindow.yml
  testinput/iodatest_obsdatavector.yml
  testinput/iodatest_obsdtype.yml
  testinput/iodatest_obsspace.yml
  testinput/iodatest_obsspace_datetime.yml
  testinput/iodatest_obsspace_out_dims_check.yml
  testinput/iodatest_obsspace_grouping.yml
  testinput/iodatest_obsspace_index_recnum.yml
  testinput/iodatest_obsspace_index_recnum_twfilt.yml
  testinput/iodatest_obsspace_invalid_numeric.yml
  testinput/iodatest_obsspace_io_pool_sondes.yml
  testinput/iodatest_obsspace_locations_qc.yml
  testinput/iodatest_obsspace_marine.yml
  testinput/iodatest_obsspace_mpi.yml
  testinput/iodatest_obsspace_odc.yml
  testinput/iodatest_obsspace_odc_atms.yml
  testinput/iodatest_obsspace_fortran.yml
  testinput/iodatest_obsspace_put_db_channels.yml
  testinput/iodatest_obsspace_put_db_channels_check.yml
  testinput/iodatest_obsspace_zero_obs.yml
  testinput/iodatest_obsspace_filter_to_zero_obs.yml
  testinput/iodatest_obsspace_fill_value.yml
  testinput/iodatest_obsvector.yml
  testinput/iodatest_obsvector_packeigen.yml
  testinput/iodatest_extendedobsspace.yml
  testinput/iodatest_extendedobsspace_halo.yml
  testinput/iodatest_sort.yml
  testinput/iodatest_writer_sondes.yml
  testinput/iodatest_odb_abiclear.yml
  testinput/iodatest_odb_ahiclear.yml
  testinput/iodatest_odb_aircraft.yml
  testinput/iodatest_odb_airs.yml
  testinput/iodatest_odb_altimeter.yml
  testinput/iodatest_odb_amsr.yml
  testinput/iodatest_odb_aod.yml
  testinput/iodatest_odb_atms.yml
  testinput/iodatest_odb_atovs.yml
  testinput/iodatest_odb_cris.yml
  testinput/iodatest_odb_crisfsr.yml
  testinput/iodatest_odb_geocloud.yml
  testinput/iodatest_odb_gmihigh.yml
  testinput/iodatest_odb_gmilow.yml
  testinput/iodatest_odb_gnssro.yml
  testinput/iodatest_odb_groundgps.yml
  testinput/iodatest_odb_hirasfsr.yml
  testinput/iodatest_odb_hloswind.yml
  testinput/iodatest_odb_iasi.yml
  testinput/iodatest_odb_mwri.yml
  testinput/iodatest_odb_mwsfy3.yml
  testinput/iodatest_odb_oceancolour.yml
  testinput/iodatest_odb_oceansound.yml
  testinput/iodatest_odb_oceansound_truncate_profiles.yml
  testinput/iodatest_odb_satsst.yml
  testinput/iodatest_odb_sattcwv.yml
  testinput/iodatest_odb_satwind.yml
  testinput/iodatest_odb_scatwind.yml
  testinput/iodatest_odb_seaice.yml
  testinput/iodatest_odb_seviriclear.yml
  testinput/iodatest_odb_sonde.yml
  testinput/iodatest_odb_sonde_truncate_profiles.yml
  testinput/iodatest_odb_ssmis.yml
  testinput/iodatest_odb_surface.yml
  testinput/iodatest_odb_surface_tcbogus.yml
  testinput/iodatest_odb_surface_with_query.yml
  testinput/iodatest_odb_surfacecloud.yml
  testinput/iodatest_odb_surfacesst.yml
  testinput/odb_default_name_map.yml
  testinput/odb_gnssro_name_map.yml
  testinput/odb_oceansound_name_map.yml
  testinput/odb_radiance_name_map.yml
  testinput/odb_sonde_name_map.yml
  testinput/iodatest_time_io.yml
)

# Create Data directory for test data and symlink files
# Set list of test file tar balls
list( APPEND ioda_test_data ioda_testinput_tier_1_2.2.0.tar.gz )

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

  # ECBUILD_DOWNLOAD_BASE_URL env var can be used to force test files and
  # crtm coef to be downloaded from other databases such as S3 instead of DASH
  # example ECBUILD_DOWNLOAD_BASE_URL=https://jedi-test-files.s3.amazonaws.com

  if( DEFINED ENV{ECBUILD_DOWNLOAD_BASE_URL} )
    set( ECBUILD_DOWNLOAD_BASE_URL "$ENV{ECBUILD_DOWNLOAD_BASE_URL}/ioda/${GIT_TAG_FUNC}" )
  else()
    set( ECBUILD_DOWNLOAD_BASE_URL https://gdex.ucar.edu/dataset/jedi-skylab/file )
  endif()


  set( DIRNAME ${GIT_TAG_FUNC} )
  set( TESTFILE_DIR_IODA "${CMAKE_SOURCE_DIR}/test-data-release/ioda/${GIT_TAG_FUNC}" )
  set( checksum "0" )

  list( APPEND IODA_DATA_DOWNLOADER_ARGS
        ${ECBUILD_DOWNLOAD_BASE_URL}
        ${CMAKE_SOURCE_DIR}/test-data-release
        ${ioda_test_data}
        ${checksum} )

  message( STATUS "Test files will be downloaded from DASH and
  saved to ${TESTFILE_DIR_IODA} for ioda")

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
# ioda engines tests
#####################################################################

ecbuild_add_test( TARGET  test_ioda_remove_hdf5_variable
                  SOURCES mains/TestRemoveHdf5Variable.cc
                  ARGS    "testinput/iodatest_remove_hdf5_variable.yml"
                  LIBS  ioda_engines ioda_test )

ecbuild_add_test( TARGET  test_ioda_remove_hdf5_variable_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking remove hdf5 variable test output"
                          remove_variable.hdf
                  TEST_DEPENDS get_ioda_test_data test_ioda_remove_hdf5_variable )

#####################################################################
# ioda utilities tests
#####################################################################

ecbuild_add_test( TARGET  test_ioda_copy_attributes
                  SOURCES mains/TestCopyAttributes.cc
                  ARGS    "testinput/iodatest_copy_attributes.yml"
                  LIBS  ioda_engines ioda_test )

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

ecbuild_add_test( TARGET  test_ioda_obsspace_datetime
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_datetime.yml"
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

ecbuild_add_test( TARGET  test_ioda_obsspace_locations_qc
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_locations_qc.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_invalid_numeric
                  SOURCES mains/TestObsSpaceInvalidNumeric.cc
                  ARGS    "testinput/iodatest_obsspace_invalid_numeric.yml"
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

if (odc_FOUND)
  ecbuild_add_test( TARGET  test_ioda_obsspace_odc
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_odc.yml"
                    TEST_DEPENDS get_ioda_test_data )
  ecbuild_add_test( TARGET  test_ioda_obsspace_odc_atms
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_odc_atms.yml"
                    TEST_DEPENDS get_ioda_test_data )
endif()

ecbuild_add_test( TARGET  test_ioda_obsspace_put_db_channels
                  SOURCES mains/TestIodaObsSpacePutDbChannels.cc
                  ARGS    "testinput/iodatest_obsspace_put_db_channels.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_put_db_channels_check
                  COMMAND test_ioda_obsspace_put_db_channels
                  ARGS    "testinput/iodatest_obsspace_put_db_channels_check.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS test_ioda_obsspace_put_db_channels get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_zero_obs
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_zero_obs.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_filter_to_zero_obs
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_filter_to_zero_obs.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_fill_value
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_fill_value.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_out_dims_check
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_CURRENT_BINARY_DIR}/test_ioda_oops_obsspace
                          testinput/iodatest_obsspace_out_dims_check.yml"
                          out_dims_check_0000.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_writer_sondes
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "${CMAKE_CURRENT_BINARY_DIR}/test_ioda_oops_obsspace
                          testinput/iodatest_writer_sondes.yml"
                          ioda_writer_sondes_0000.nc4
                  TEST_DEPENDS get_ioda_test_data )

# This test creates four output files (7 tasks, 4 tasks in the io pool)
# and the following 4 tests check the output files.
ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes
                  MPI     7
                  COMMAND time_IodaIO.x
                  ARGS    "testinput/iodatest_obsspace_io_pool_sondes.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_file_0000
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_out_0000.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_file_0001
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_out_0001.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_file_0002
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_out_0002.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_file_0003
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_out_0003.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes)

# IODA ObsSpace class - Fortran interface test
add_fctest( TARGET  test_ioda_obsspace_fortran
            SOURCES ioda/obsspace.F90
            ARGS    --config testinput/iodatest_obsspace_fortran.yml
            LINKER_LANGUAGE Fortran
            LIBS  ioda_test
            TEST_DEPENDS get_ioda_test_data )

# IODA ObsSpace sorting of obs groups
ecbuild_add_test( TARGET  test_ioda_sort
                  SOURCES mains/TestSort.cc
                  ARGS    "testinput/iodatest_sort.yml"
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
                  TEST_DEPENDS get_ioda_test_data test_ioda_oops_obsvector )

# IODA ObsVector class
ecbuild_add_test( TARGET  test_ioda_obsvector
                  MPI     4
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS    "testinput/iodatest_obsvector.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_oops_obsvector test_ioda_oops_obsdatavector )

# IODA ObsVector class (packEigen method)
ecbuild_add_test( TARGET  test_ioda_obsvector_packeigen
                  MPI     4
                  SOURCES mains/TestIodaObsVectorPackEigen.cc
                  ARGS    "testinput/iodatest_obsvector_packeigen.yml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

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

# this test creates the output files
ecbuild_add_test( TARGET  test_ioda_time_io
                  COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                  ARGS    "testinput/iodatest_time_io.yml"
                  TEST_DEPENDS get_ioda_test_data )

# the following tests compare the output files
ecbuild_add_test( TARGET  test_ioda_time_io_cmp_sondes
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io sondes"
                          sondes_obs_2018041500_m_time_0000.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_amsua_n19
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io AMSU-A NOAA19"
                          amsua_n19_obs_2018041500_m_time_0000.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

#####################################################################
# Test the ioda-upgrade.x app
#####################################################################

ecbuild_add_test( TARGET  test_ioda_upgrader_amsua_n19
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_amsua_n19.nc4
                          testoutput/upgrader_amsua_n19.nc4"
                          upgrader_amsua_n19.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_gmi_gpm
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_gmi_gpm.nc4
                          testoutput/upgrader_gmi_gpm.nc4"
                          upgrader_gmi_gpm.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_mhs_metop-b
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_mhs_metop-b.nc4
                          testoutput/upgrader_mhs_metop-b.nc4"
                          upgrader_mhs_metop-b.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_sondes
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_sondes.nc4
                          testoutput/upgrader_sondes.nc4"
                          upgrader_sondes.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_aod
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_aod.nc4
                          testoutput/upgrader_aod.nc4"
                          upgrader_aod.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_gnssro
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_gnssro.nc4
                          testoutput/upgrader_gnssro.nc4"
                          upgrader_gnssro.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_met_office_gpsro
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_met_office_gpsro.nc4
                          testoutput/upgrader_met_office_gpsro.nc4"
                          upgrader_met_office_gpsro.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_zero_obs
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade.x
                          Data/testinput_tier_1/upgrader_zero_obs.nc4
                          testoutput/upgrader_zero_obs.nc4"
                          upgrader_zero_obs.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_amsua_n19_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: amsua_n19 netcdf compatibility check"
                          upgrader_amsua_n19.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_amsua_n19 )

ecbuild_add_test( TARGET  test_ioda_upgrader_gmi_gpm_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: gmi_gpm netcdf compatibility check"
                          upgrader_gmi_gpm.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_gmi_gpm )

ecbuild_add_test( TARGET  test_ioda_upgrader_mhs_metop-b_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: mhs_metop-b netcdf compatibility check"
                          upgrader_mhs_metop-b.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_mhs_metop-b )

ecbuild_add_test( TARGET  test_ioda_upgrader_sondes_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: sondes netcdf compatibility check"
                          upgrader_sondes.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_sondes )

ecbuild_add_test( TARGET  test_ioda_upgrader_aod_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: aod netcdf compatibility check"
                          upgrader_aod.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_aod )

ecbuild_add_test( TARGET  test_ioda_upgrader_gnssro_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: gnssro netcdf compatibility check"
                          upgrader_gnssro.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_gnssro )

ecbuild_add_test( TARGET  test_ioda_upgrader_met_office_gpsro_nc_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: met_office_gpsro netcdf compatibility check"
                          upgrader_met_office_gpsro.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_met_office_gpsro )

ecbuild_add_test( TARGET  test_ioda_upgrader_zero_obs_compat
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                  ARGS    netcdf
                          "echo INFO: zero_obs netcdf compatibility check"
                          upgrader_zero_obs.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_upgrader_zero_obs )

if (odc_FOUND)
  ecbuild_add_test( TARGET  test_ioda-convert_aircraft_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Aircraft
                            Data/testinput_tier_1/aircraft.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_aircraft.yml"
                            test-Aircraft.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_aircraft_mixed_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Mixed-Aircraft
                            Data/testinput_tier_1/aircraft-mixed-elements.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_aircraft.yml"
                            test-Mixed-Aircraft.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surface_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Surface
                            Data/testinput_tier_1/surface.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_surface.yml"
                            test-Surface.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surface_tcbogus_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Surface-TCBOGUS
                            Data/testinput_tier_1/surface-tcbogus.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_surface_tcbogus.yml"
                            test-Surface-TCBOGUS.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surface_with_query_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Surface-with-query
                            Data/testinput_tier_1/surface.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_surface_with_query.yml"
                            test-Surface-with-query.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surface_missing_odc
                    TYPE    SCRIPT
                    COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                    ARGS    Surface-missing
                            Data/testinput_tier_1/surface-missing.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_surface.yml
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surface_empty_odc
                    TYPE    SCRIPT
                    COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                    ARGS    Surface-empty
                            Data/testinput_tier_1/surface-empty.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_surface.yml
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde
                            Data/testinput_tier_1/sonde.odb
                            testinput/odb_sonde_name_map.yml testinput/iodatest_odb_sonde.yml"
                            test-Sonde.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_artificial_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde-artificial
                            Data/testinput_tier_1/sonde-artificial.odb
                            testinput/odb_sonde_name_map.yml testinput/iodatest_odb_sonde.yml"
                            test-Sonde-artificial.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_truncate_profiles_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde-truncate-profiles
                            Data/testinput_tier_1/sonde.odb
                            testinput/odb_sonde_name_map.yml testinput/iodatest_odb_sonde_truncate_profiles.yml"
                            test-Sonde-truncate-profiles.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_abiclear_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            ABIClear
                            Data/testinput_tier_1/abiclear.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_abiclear.yml"
                            test-ABIClear.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_ahiclear_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            AHIClear
                            Data/testinput_tier_1/ahiclear.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_ahiclear.yml"
                            test-AHIClear.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_amsr_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            AMSR
                            Data/testinput_tier_1/amsr.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_amsr.yml"
                            test-AMSR.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_airs_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            AIRS
                            Data/testinput_tier_1/airs.odb
                            testinput/odb_radiance_name_map.yml testinput/iodatest_odb_airs.yml"
                            test-AIRS.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_atovs_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            ATOVS
                            Data/testinput_tier_1/atovs.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_atovs.yml"
                            test-ATOVS.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_gmihigh_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            GMIHigh
                            Data/testinput_tier_1/gmihigh.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_gmihigh.yml"
                            test-GMIHigh.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_gmilow_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            GMILow
                            Data/testinput_tier_1/gmilow.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_gmilow.yml"
                            test-GMILow.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_ssmis_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SSMIS
                            Data/testinput_tier_1/ssmis.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_ssmis.yml"
                            test-SSMIS.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_satwind_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Satwind
                            Data/testinput_tier_1/satwind.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_satwind.yml"
                            test-Satwind.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_scatwind_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Scatwind
                            Data/testinput_tier_1/scatwind.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_scatwind.yml"
                            test-Scatwind.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_gnssro_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            GNSSRO
                            Data/testinput_tier_1/gnssro.odb
                            testinput/odb_gnssro_name_map.yml testinput/iodatest_odb_gnssro.yml"
                            test-GNSSRO.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_gnssro_profile_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            GNSSRO-profile
                            Data/testinput_tier_1/gnssro.odb
                            testinput/odb_gnssro_name_map.yml testinput/iodatest_odb_gnssro.yml 333"
                            test-GNSSRO.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_groundgps_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            GroundGPS
                            Data/testinput_tier_1/groundgps.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_groundgps.yml"
                            test-GroundGPS.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_aod_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            AOD
                            Data/testinput_tier_1/aod.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_aod.yml"
                            test-AOD.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_cris_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            CrIS
                            Data/testinput_tier_1/cris.odb
                            testinput/odb_radiance_name_map.yml testinput/iodatest_odb_cris.yml"
                            test-CrIS.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_crisfsr_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            CrISFSR
                            Data/testinput_tier_1/crisfsr.odb
                            testinput/odb_radiance_name_map.yml testinput/iodatest_odb_crisfsr.yml"
                            test-CrISFSR.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_hirasfsr_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            HIRASFSR
                            Data/testinput_tier_1/hirasfsr.odb
                            testinput/odb_radiance_name_map.yml  testinput/iodatest_odb_hirasfsr.yml"
                            test-HIRASFSR.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_hloswind_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            HLOSWind
                            Data/testinput_tier_1/hloswind.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_hloswind.yml"
                            test-HLOSWind.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_mwri_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            MWRI
                            Data/testinput_tier_1/mwri.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_mwri.yml"
                            test-MWRI.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_iasi_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            IASI
                            Data/testinput_tier_1/iasi.odb
                            testinput/odb_radiance_name_map.yml testinput/iodatest_odb_iasi.yml"
                            test-IASI.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_mwsfy3_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            MWSFY3
                            Data/testinput_tier_1/mwsfy3.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_mwsfy3.yml"
                            test-MWSFY3.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sattcwv_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SatTCWV
                            Data/testinput_tier_1/sattcwv.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_sattcwv.yml"
                            test-SatTCWV.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_seviriclear_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SEVIRIClear
                            Data/testinput_tier_1/seviriclear.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_seviriclear.yml"
                            test-SEVIRIClear.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_atms_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            ATMS
                            Data/testinput_tier_1/atms.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_atms.yml"
                            test-ATMS.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_geocloud_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            GeoCloud
                            Data/testinput_tier_1/geocloud.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_geocloud.yml"
                            test-GeoCloud.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surfacecloud_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SurfaceCloud
                            Data/testinput_tier_1/surfacecloud.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_surfacecloud.yml"
                            test-SurfaceCloud.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_altimeter_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Altimeter
                            Data/testinput_tier_1/altimeter.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_altimeter.yml"
                            test-Altimeter.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_oceancolour_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            OceanColour
                            Data/testinput_tier_1/oceancol.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_oceancolour.yml"
                            test-OceanColour.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_satsst_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SatSST
                            Data/testinput_tier_1/satsst.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_satsst.yml"
                            test-SatSST.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_seaice_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SeaIce
                            Data/testinput_tier_1/seaice.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_seaice.yml"
                            test-SeaIce.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surfacesst_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SurfaceSST
                            Data/testinput_tier_1/surfacesst.odb
                            testinput/odb_default_name_map.yml testinput/iodatest_odb_surfacesst.yml"
                            test-SurfaceSST.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_oceansound_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            OceanSound
                            Data/testinput_tier_1/oceansound.odb
                            testinput/odb_oceansound_name_map.yml testinput/iodatest_odb_oceansound.yml"
                            test-OceanSound.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_oceansound_truncate_profiles_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            OceanSound-truncate-profiles
                            Data/testinput_tier_1/oceansound.odb
                            testinput/odb_oceansound_name_map.yml testinput/iodatest_odb_oceansound_truncate_profiles.yml"
                            test-OceanSound-truncate-profiles.hdf
                    TEST_DEPENDS get_ioda_test_data )

  add_test_checking_odb_to_netcdf_conversion_results_against_odb_sql_queries(
    TEST_NAME varno_independent_column
    ODB_FILE  sonde.odb )

  add_test_checking_odb_to_netcdf_conversion_results_against_odb_sql_queries(
    TEST_NAME varno_dependent_column
    ODB_FILE  sonde.odb )

  add_test_checking_odb_to_netcdf_conversion_results_against_odb_sql_queries(
    TEST_NAME varno_independent_bitfield_column
    ODB_FILE  sonde.odb )

  add_test_checking_odb_to_netcdf_conversion_results_against_odb_sql_queries(
    TEST_NAME varno_dependent_bitfield_column
    ODB_FILE  sonde.odb )

  add_test_checking_odb_to_netcdf_conversion_results_against_odb_sql_queries(
    TEST_NAME varno_independent_bitfield_column_members
    ODB_FILE  sonde.odb )

  add_test_checking_odb_to_netcdf_conversion_results_against_odb_sql_queries(
    TEST_NAME varno_dependent_bitfield_column_members
    ODB_FILE  sonde.odb )
endif()

#####################################################################
# String-to-enum conversion tests
#####################################################################

ecbuild_add_test( TARGET  test_ioda_obsdtype
                  SOURCES mains/TestIodaObsDtype.cc
                  ARGS    "testinput/iodatest_obsdtype.yml"
                  LIBS    ioda_test )

ecbuild_add_test( TARGET  test_ioda_fileformat
                  SOURCES mains/TestIodaFileFormat.cc
                  ARGS    "testinput/iodatest_fileformat.yml"
                  LIBS    ioda_test )

#####################################################################
# ioda-validate code coverage test
#####################################################################
# Actual file tests are in ufo-data and in ioda-converters.
# - IODA_YAML_ROOT is set in the top-level ioda CMakeLists.txt file.
# - Future: IODA_DATA_TEST_ROOT is set by the ioda-data project.
ecbuild_add_test(
	TARGET test_ioda-validate
	COMMAND ioda-validate.x
	ARGS "${IODA_YAML_ROOT}/validation/ObsSpace.yaml"
             "Data/testinput_tier_1/sample_hofx_output_amsua_n19.nc4"
	     # Future: "${IODA_DATA_TEST_ROOT}/testinput_tier_1/sample_hofx_output_amsua_n19.nc4"
	)

#####################################################################
# Set up the testinput and testoutput directories
#####################################################################

# Create symlinks to input YAML configuration files
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
