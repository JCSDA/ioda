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
                                testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_name_map.yaml
                                testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_query.yaml
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
      testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_name_map.yaml
      testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_query.yaml
      testinput/iodatest_odb_convert_${_PAR_TEST_NAME}_comparison.conf )
    set( ioda_test_input ${ioda_test_input} PARENT_SCOPE )
  endfunction()
endif()

# The list of all test input config files, which need to be symlink from the build folder.
# (These symlink are created at the end of this file, to give CMake functions setting up certain
# tests a chance to append items to this list).
list( APPEND ioda_test_input
  testinput/iodatest.yaml
  testinput/iodatest_fileformat.yaml
  testinput/iodatest_remove_hdf5_variable.yaml
  testinput/iodatest_copy_attributes.yaml
  testinput/iodatest_obserror.yaml
  testinput/iodatest_distribution.yaml
  testinput/iodatest_distribution_masterandreplica_mpi_2.yaml
  testinput/iodatest_distribution_masterandreplica_mpi_3.yaml
  testinput/iodatest_distribution_masterandreplica_mpi_4.yaml
  testinput/iodatest_distribution_methods.yaml
  testinput/iodatest_distribution_timewindow.yaml
  testinput/iodatest_engine_construct_utils.yaml
  testinput/iodatest_engine_construct_utils_odc.yaml
  testinput/iodatest_engine_factory.yaml
  testinput/iodatest_engine_factory_odc.yaml
  testinput/iodatest_engine_uniquify_file_name.yaml
  testinput/iodatest_io_pool_factory.yaml
  testinput/iodatest_obsdatavector.yaml
  testinput/iodatest_obsdtype.yaml
  testinput/iodatest_obsspace.yaml
  testinput/iodatest_obsspace_reader_pool.yaml
  testinput/iodatest_obsspace_datetime.yaml
  testinput/iodatest_obsspace_dist_write.yaml
  testinput/iodatest_obsspace_default_fill_values.yaml
  testinput/iodatest_obsspace_empty_obs_file.yaml
  testinput/iodatest_obsspace_empty_obs_file_mpi.yaml
  testinput/iodatest_obsspace_empty_obs_file_reader_pool.yaml
  testinput/iodatest_obsspace_empty_obs_file_mpi_reader_pool.yaml
  testinput/iodatest_obsspace_out_dims_check.yaml
  testinput/iodatest_obsspace_grouping.yaml
  testinput/iodatest_obsspace_grouping_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum.yaml
  testinput/iodatest_obsspace_index_recnum_twfilt.yaml
  testinput/iodatest_obsspace_index_recnum_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum_twfilt_reader_pool.yaml
  testinput/iodatest_obsspace_invalid_numeric.yaml
  testinput/iodatest_obsspace_io_pool_sondes_single_file.yaml
  testinput/iodatest_obsspace_io_pool_sondes_multi_files.yaml
  testinput/iodatest_obsspace_locations_qc.yaml
  testinput/iodatest_obsspace_locations_qc_twfilt.yaml
  testinput/iodatest_obsspace_marine.yaml
  testinput/iodatest_obsspace_mpi.yaml
  testinput/iodatest_obsspace_mpi_reader_pool.yaml
  testinput/iodatest_obsspace_odc.yaml
  testinput/iodatest_obsspace_odc_reader_pool.yaml
  testinput/iodatest_obsspace_odc_atms.yaml
  testinput/iodatest_obsspace_odc_atms_reader_pool.yaml
  testinput/iodatest_obsspace_fortran.yaml
  testinput/iodatest_obsspace_out_odc.yaml
  testinput/iodatest_obsspace_out_odc_mpi_2.yaml
  testinput/iodatest_obsspace_put_db_channels.yaml
  testinput/iodatest_obsspace_put_db_channels_check.yaml
  testinput/iodatest_obsspace_zero_obs.yaml
  testinput/iodatest_obsspace_filter_to_zero_obs.yaml
  testinput/iodatest_obsspace_zero_obs_reader_pool.yaml
  testinput/iodatest_obsspace_filter_to_zero_obs_reader_pool.yaml
  testinput/iodatest_obsspace_fill_value.yaml
  testinput/iodatest_obsvector.yaml
  testinput/iodatest_obsvector_missing_locs.yaml
  testinput/iodatest_obsvector_packeigen.yaml
  testinput/iodatest_extendedobsspace.yaml
  testinput/iodatest_extendedobsspace_halo.yaml
  testinput/iodatest_odb_gnssro_profile.yaml
  testinput/iodatest_odb_surface_with_query.yaml
  testinput/iodatest_sort.yaml
  testinput/iodatest_writer_sondes.yaml
  testinput/iodatest_time_io.yaml
  testinput/iodatest_time_io_odc.yaml
  testinput/iodatest_time_io_reader_pool.yaml
  testinput/iodatest_time_io_reader_pool_odc.yaml
)

# Create Data directory for test data and symlink files
# Set list of test file tar balls
list( APPEND ioda_test_data ioda_testinput_tier_1_2.5.0.tar.gz )

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
elseif( DEFINED ENV{IODA_TESTFILES} )
  # Any branch of IODA is being built.
  # ioda-data repository is already cloned by bundle/CMakeLists.txt.
  # Link test files in ioda-data repo to build directory.
  # get_ioda_test_data checks the existence of test file directory.
  set( TESTFILE_DIR_IODA $ENV{IODA_TESTFILES} )
  list( APPEND IODA_DATA_DOWNLOADER_ARGS
      ${TESTFILE_DIR_IODA} )
  set (IODA_DATA_DOWNLOADER ioda_data_checker.py )
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
# Distribution tests
#####################################################################
ecbuild_add_test( TARGET  test_ioda_distribution
                  MPI     4
                  SOURCES mains/TestDistribution.cc
                  ARGS    "testinput/iodatest_distribution.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_distribution_masterandreplica_mpi_2
                  MPI     2
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_masterandreplica_mpi_2.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_distribution_masterandreplica_mpi_3
                  MPI     3
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_masterandreplica_mpi_3.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_distribution_masterandreplica_mpi_4
                  MPI     4
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_masterandreplica_mpi_4.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_distribution_timewindow
                  MPI     4
                  COMMAND test_ioda_distribution
                  ARGS    "testinput/iodatest_distribution_timewindow.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# Disrtibution method tests
ecbuild_add_test( TARGET  test_ioda_distribution_methods
                  MPI     4
                  SOURCES mains/TestDistributionMethods.cc
                  ARGS    "testinput/iodatest_distribution_methods.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ioda engines tests
#####################################################################

ecbuild_add_test( TARGET  test_ioda_remove_hdf5_variable
                  SOURCES mains/TestRemoveHdf5Variable.cc
                  ARGS    "testinput/iodatest_remove_hdf5_variable.yaml"
                  LIBS  ioda_engines ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_remove_hdf5_variable_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking remove hdf5 variable test output"
                          remove_variable.hdf
                  TEST_DEPENDS get_ioda_test_data test_ioda_remove_hdf5_variable )

ecbuild_add_test( TARGET  test_ioda_engine_construct_utils
                  SOURCES mains/TestEngineConstructUtils.cc
                  ARGS    "testinput/iodatest_engine_construct_utils.yaml"
                  LIBS  ioda_engines ioda_test
                  TEST_DEPENDS get_ioda_test_data )
if (odc_FOUND)
  ecbuild_add_test( TARGET  test_ioda_engine_construct_utils_odc
                    SOURCES mains/TestEngineConstructUtils.cc
                    ARGS    "testinput/iodatest_engine_construct_utils_odc.yaml"
                    LIBS  ioda_engines ioda_test
                    TEST_DEPENDS get_ioda_test_data )
endif()

ecbuild_add_test( TARGET  test_ioda_engine_factory
                  SOURCES mains/TestEngineFactory.cc
                  ARGS    "testinput/iodatest_engine_factory.yaml"
                  LIBS  ioda_engines ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_engine_uniquify_file_name
                  SOURCES mains/TestEngineUniquifyFileName.cc
                  ARGS    "testinput/iodatest_engine_uniquify_file_name.yaml"
                  LIBS  ioda_engines ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_io_pool_factory
                  SOURCES mains/TestIoPoolFactory.cc
                  ARGS    "testinput/iodatest_io_pool_factory.yaml"
                  LIBS  ioda_engines ioda_test
                  TEST_DEPENDS get_ioda_test_data )

if (odc_FOUND)
    ecbuild_add_test( TARGET  test_ioda_engine_factory_odc
                      COMMAND test_ioda_engine_factory
                      ARGS    "testinput/iodatest_engine_factory_odc.yaml"
                      LIBS  ioda_engines ioda_test
                      TEST_DEPENDS get_ioda_test_data test_ioda_engine_factory )
endif()

#####################################################################
# ioda utilities tests
#####################################################################

ecbuild_add_test( TARGET  test_ioda_copy_attributes
                  SOURCES mains/TestCopyAttributes.cc
                  ARGS    "testinput/iodatest_copy_attributes.yaml"
                  LIBS  ioda_engines ioda_test )

#####################################################################
# ObsSpace tests
#####################################################################

# OOPS ObsSpace interface
ecbuild_add_test( TARGET  test_ioda_oops_obsspace
                  SOURCES mains/TestObsSpace.cc
                  ARGS    "testinput/iodatest.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA ObsSpace class
ecbuild_add_test( TARGET  test_ioda_obsspace
                  SOURCES mains/TestIodaObsSpace.cc
                  ARGS    "testinput/iodatest_obsspace.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_reader_pool
                  SOURCES mains/TestIodaObsSpace.cc
                  ARGS    "testinput/iodatest_obsspace_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

if (odc_FOUND)
  ecbuild_add_test( TARGET  test_ioda_clean_obsspace_out_odc_files
                    COMMAND rm
                    ARGS    "-f" "testoutput/diagout-aircraft_0000.odc" "testoutput/diagout-atms_0000.odc" "testoutput/diagout-gmi_0000.odc" "testoutput/diagout-amsua_0000.odc" "testoutput/diagout-sonde_0000.odc" "testoutput/diagout-aod_0000.odc" "testoutput/diagout-iasi_0000.odc" "testoutput/diagout-aod-testdatasets_0000.odc" "testoutput/diagout-aod-testdatasets_0001.odc")
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_out_odc.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS get_ioda_test_data test_ioda_clean_obsspace_out_odc_files )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_mpi_2
                    MPI 2
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_out_odc_mpi_2.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS get_ioda_test_data test_ioda_clean_obsspace_out_odc_files )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aircraft
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aircraft_0000.odc" "Data/testinput_tier_1/test_reference/diagout-aircraft_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_atms
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-atms_0000.odc" "Data/testinput_tier_1/test_reference/diagout-atms_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_gmi
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-gmi_0000.odc" "Data/testinput_tier_1/test_reference/diagout-gmi_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_amsua
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-amsua_0000.odc" "Data/testinput_tier_1/test_reference/diagout-amsua_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_sonde
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-sonde_0000.odc" "Data/testinput_tier_1/test_reference/diagout-sonde_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod_0000.odc" "Data/testinput_tier_1/test_reference/diagout-aod_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_iasi
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-iasi_0000.odc" "Data/testinput_tier_1/test_reference/diagout-iasi_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod_testdatsets_0000
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod-testdatasets_0000.odc" "Data/testinput_tier_1/test_reference/diagout-aod-testdatasets_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_mpi_2 )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod_testdatsets_0001
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod-testdatasets_0001.odc" "Data/testinput_tier_1/test_reference/diagout-aod-testdatasets_0001.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_mpi_2 )

endif()

ecbuild_add_test( TARGET  test_ioda_obsspace_datetime
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_datetime.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_empty_obs_file
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_empty_obs_file.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_empty_obs_file_mpi
                  MPI 4
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_empty_obs_file_mpi.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_empty_obs_file_reader_pool
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_empty_obs_file_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_empty_obs_file_mpi_reader_pool
                  MPI 4
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_empty_obs_file_mpi_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_grouping
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_grouping.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_grouping_reader_pool
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_grouping_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum
                  SOURCES mains/TestIodaObsSpaceIndexRecnum.cc
                  ARGS    "testinput/iodatest_obsspace_index_recnum.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_mpi_2
                  MPI 2
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_mpi_4
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_mpi_2
                  MPI 2
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_mpi_4
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_reader_pool
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_mpi_2_reader_pool
                  MPI 2
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_mpi_4_reader_pool
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_reader_pool
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_mpi_2_reader_pool
                  MPI 2
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_mpi_4_reader_pool
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_locations_qc
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_locations_qc.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_locations_qc_twfilt
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_locations_qc_twfilt.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_invalid_numeric
                  SOURCES mains/TestObsSpaceInvalidNumeric.cc
                  ARGS    "testinput/iodatest_obsspace_invalid_numeric.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_mpi
                  MPI     2
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_mpi.yaml"
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_mpi_reader_pool
                  MPI     2
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_mpi_reader_pool.yaml"
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_marine
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_marine.yaml"
                  TEST_DEPENDS get_ioda_test_data )

if (odc_FOUND)
  ecbuild_add_test( TARGET  test_ioda_obsspace_odc
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_odc.yaml"
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda_obsspace_odc_reader_pool
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_odc_reader_pool.yaml"
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda_obsspace_odc_atms
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_odc_atms.yaml"
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda_obsspace_odc_atms_reader_pool
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_odc_atms_reader_pool.yaml"
                    TEST_DEPENDS get_ioda_test_data )
endif()

ecbuild_add_test( TARGET  test_ioda_obsspace_put_db_channels
                  SOURCES mains/TestIodaObsSpacePutDbChannels.cc
                  ARGS    "testinput/iodatest_obsspace_put_db_channels.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_put_db_channels_check
                  COMMAND test_ioda_obsspace_put_db_channels
                  ARGS    "testinput/iodatest_obsspace_put_db_channels_check.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS test_ioda_obsspace_put_db_channels get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_zero_obs
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_zero_obs.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_filter_to_zero_obs
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_filter_to_zero_obs.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_zero_obs_reader_pool
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_zero_obs_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_filter_to_zero_obs_reader_pool
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_filter_to_zero_obs_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_fill_value
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_fill_value.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_default_fill_values
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_default_fill_values.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_default_fill_values_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io default fill values"
                          default_fill_values_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_default_fill_values )

ecbuild_add_test( TARGET  test_ioda_obsspace_out_dims_check
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_CURRENT_BINARY_DIR}/test_ioda_oops_obsspace
                          testinput/iodatest_obsspace_out_dims_check.yaml"
                          out_dims_check.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_writer_sondes
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "${CMAKE_CURRENT_BINARY_DIR}/test_ioda_oops_obsspace
                          testinput/iodatest_writer_sondes.yaml"
                          ioda_writer_sondes.nc4
                  TEST_DEPENDS get_ioda_test_data )

# This tests creates a single output file (7 tasks, 4 tasks in the io pool)
# and the following test checks the output file.
ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_single_file
                  MPI     7
                  COMMAND time_IodaIO.x
                  ARGS    "testinput/iodatest_obsspace_io_pool_sondes_single_file.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_single_file_check
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_single_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes_single_file)


# This test creates four output files (7 tasks, 4 tasks in the io pool)
# and the following 4 tests check the output files. The only difference in this
# set of tests and the tests above is that the yaml file uses the "write multiple files"
# control.
ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_multi_files
                  MPI     7
                  COMMAND time_IodaIO.x
                  ARGS    "testinput/iodatest_obsspace_io_pool_sondes_multi_files.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_multi_file_check_0000
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_multi_out_0000.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes_multi_files)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_multi_file_check_0001
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_multi_out_0001.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes_multi_files)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_multi_file_check_0002
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_multi_out_0002.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes_multi_files)

ecbuild_add_test( TARGET  test_ioda_obsspace_io_pool_sondes_multi_file_check_0003
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking io pool io sondes"
                          io_pool_sondes_multi_out_0003.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_io_pool_sondes_multi_files)

# These tests check that the writer does not create duplicate obs for all of the
# distribution types (currently, RoundRobin, Inefficient, Halo and Atlas).
# The first test creates 4 files each based on the 4 different distribution types,
# and the next 4 tests check those files. Inefficient, Halo and Atlas preserve the
# order of the obs in the input file so they can be checked with the same reference file.
# RoundRobin interleaves the input obs order, so it needs its own reference file.
ecbuild_add_test( TARGET  test_ioda_obsspace_dist_write
                  MPI     4
                  COMMAND time_IodaIO.x
                  ARGS    "testinput/iodatest_obsspace_dist_write.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_obsspace_dist_write_round_robin
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking distributed write - Round Robin"
                          dist_write_round_robin_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_dist_write)

ecbuild_add_test( TARGET  test_ioda_obsspace_dist_write_inefficient
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking distributed write - Inefficient"
                          dist_write_inefficient_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_dist_write)

ecbuild_add_test( TARGET  test_ioda_obsspace_dist_write_halo
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking distributed write - Halo"
                          dist_write_halo_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_dist_write)

ecbuild_add_test( TARGET  test_ioda_obsspace_dist_write_atlas
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking distributed write - Atlas"
                          dist_write_atlas_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_dist_write)

# IODA ObsSpace class - Fortran interface test
add_fctest( TARGET  test_ioda_obsspace_fortran
            SOURCES ioda/obsspace.F90
            ARGS    --config testinput/iodatest_obsspace_fortran.yaml
            LINKER_LANGUAGE Fortran
            LIBS  ioda_test
            TEST_DEPENDS get_ioda_test_data )

# IODA ObsSpace sorting of obs groups
ecbuild_add_test( TARGET  test_ioda_sort
                  SOURCES mains/TestSort.cc
                  ARGS    "testinput/iodatest_sort.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# IODA extended ObsSpace
ecbuild_add_test( TARGET  test_ioda_extendedobsspace
                  SOURCES mains/TestExtendedObsSpace.cc
                  ARGS    "testinput/iodatest_extendedobsspace.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_MPI_2
                  MPI     2
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_MPI_3
                  MPI     3
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_halo_MPI_2
                  MPI     2
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace_halo.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_extendedobsspace_halo_MPI_3
                  MPI     3
                  COMMAND test_ioda_extendedobsspace
                  ARGS    "testinput/iodatest_extendedobsspace_halo.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsVector tests
#####################################################################

# OOPS ObsVector interface
ecbuild_add_test( TARGET  test_ioda_oops_obsvector
                  MPI     4
                  SOURCES mains/TestObsVector.cc
                  ARGS    "testinput/iodatest_obsvector.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_oops_obsdatavector
                  SOURCES mains/TestObsDataVector.cc
                  ARGS    "testinput/iodatest_obsvector.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_oops_obsvector )

# IODA ObsVector class
ecbuild_add_test( TARGET  test_ioda_obsvector
                  MPI     4
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS    "testinput/iodatest_obsvector.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_oops_obsvector test_ioda_oops_obsdatavector )

ecbuild_add_test( TARGET  test_ioda_obsvector_missing_locs
                  MPI     4
                  COMMAND test_ioda_obsvector
                  ARGS    "testinput/iodatest_obsvector_missing_locs.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_oops_obsvector test_ioda_oops_obsdatavector )

# IODA ObsVector class (packEigen method)
ecbuild_add_test( TARGET  test_ioda_obsvector_packeigen
                  MPI     4
                  SOURCES mains/TestIodaObsVectorPackEigen.cc
                  ARGS    "testinput/iodatest_obsvector_packeigen.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsDataVector tests
#####################################################################

# IODA ObsDataVector class
ecbuild_add_test( TARGET  test_ioda_obsdatavector
                  MPI     2
                  SOURCES mains/TestIodaObsDataVector.cc
                  ARGS    "testinput/iodatest_obsdatavector.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# ObsErrorCovariance tests
#####################################################################

# OOPS ObsErrorCovariance interface (dependent on ObsVector)
ecbuild_add_test( TARGET  test_ioda_oops_obserrorcovariance
                  SOURCES mains/TestObsErrorCovariance.cc
                  ARGS    "testinput/iodatest_obserror.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

#####################################################################
# Test the time_IodaIO.x app
#####################################################################

# this test removes the output files from prior testing
ecbuild_add_test( TARGET  test_ioda_clean_time_io_files
                  COMMAND rm
                  ARGS    "-f"
                          "testoutput/amsua_n19_obs_2018041500_m_time.nc4"
                          "testoutput/sondes_obs_2018041500_m_time.nc4"
                          "testoutput/gnssro_obs_2021080606_s_time.nc4"
                          "testoutput/amsua_n19_obs_2018041500_m_time_reader_pool.nc4"
                          "testoutput/sondes_obs_2018041500_m_time_reader_pool.nc4"
                          "testoutput/gnssro_obs_2021080606_s_time_reader_pool.nc4" )

# this test creates new output files
ecbuild_add_test( TARGET  test_ioda_time_io
                  COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                  ARGS    "testinput/iodatest_time_io.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_time_io_files )

# the following tests compare the output files
ecbuild_add_test( TARGET  test_ioda_time_io_cmp_sondes
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io sondes"
                          sondes_obs_2018041500_m_time.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_amsua_n19
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io AMSU-A NOAA19"
                          amsua_n19_obs_2018041500_m_time.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_gnssro
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io GNSSRO"
                          gnssro_obs_2021080606_s_time.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

# this test creates new output files
ecbuild_add_test( TARGET  test_ioda_time_io_reader_pool
                  COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                  ARGS    "testinput/iodatest_time_io_reader_pool.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_time_io_files )

# the following tests compare the output files
ecbuild_add_test( TARGET  test_ioda_time_io_cmp_sondes_reader_pool
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io (reader pool) sondes"
                          sondes_obs_2018041500_m_time_reader_pool.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_amsua_n19_reader_pool
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io (reader pool) AMSU-A NOAA19"
                          amsua_n19_obs_2018041500_m_time_reader_pool.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_gnssro_reader_pool
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "echo Checking time io (reader pool) GNSSRO"
                          gnssro_obs_2021080606_s_time_reader_pool.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool)

if (odc_FOUND)
  # this test removes the output files from prior testing
  ecbuild_add_test( TARGET  test_ioda_clean_time_io_files_odc
                    COMMAND rm
                    ARGS    "-f"
                            "testoutput/aircraft_time.odb"
                            "testouput/aircraft_time_reader_pool.odb"
                            "testoutput/atms_time.odb"
                            "testoutput/atms_reader_pool.odb" )

  # this test creates new output files
  ecbuild_add_test( TARGET  test_ioda_time_io_odc
                    COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                    ARGS    "testinput/iodatest_time_io_odc.yaml"
                    TEST_DEPENDS get_ioda_test_data test_ioda_clean_time_io_files_odc )

  # the following tests compare the output files
  ecbuild_add_test( TARGET  test_ioda_time_io_cmp_aircraft_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            odb
                            "echo Checking time io aircraft"
                            aircraft_time_0000.odb
                    TEST_DEPENDS get_ioda_test_data test_ioda_time_io_odc)

  ecbuild_add_test( TARGET  test_ioda_time_io_cmp_atms_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            odb
                            "echo Checking time io ATMS"
                            atms_time_0000.odb
                    TEST_DEPENDS get_ioda_test_data test_ioda_time_io_odc)

  # this test creates new output files
  ecbuild_add_test( TARGET  test_ioda_time_io_reader_pool_odc
                    COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                    ARGS    "testinput/iodatest_time_io_reader_pool_odc.yaml"
                    TEST_DEPENDS get_ioda_test_data test_ioda_clean_time_io_files_odc )

  # the following tests compare the output files
  ecbuild_add_test( TARGET  test_ioda_time_io_cmp_aircraft_reader_pool_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            odb
                            "echo Checking time io (reader pool) aircraft"
                            aircraft_time_reader_pool_0000.odb
                    TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_odc)

  ecbuild_add_test( TARGET  test_ioda_time_io_cmp_atms_reader_pool_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            odb
                            "echo Checking time io (reader pool) ATMS"
                            atms_time_reader_pool_0000.odb
                    TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_odc)
endif()

#####################################################################
# Test the ioda-upgrade-v1-to-v2.x app
#####################################################################

ecbuild_add_test( TARGET  test_ioda_upgrader_amsua_n19
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
                          Data/testinput_tier_1/upgrader_amsua_n19.nc4
                          testoutput/upgrader_amsua_n19.nc4"
                          upgrader_amsua_n19.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_gmi_gpm
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
                          Data/testinput_tier_1/upgrader_gmi_gpm.nc4
                          testoutput/upgrader_gmi_gpm.nc4"
                          upgrader_gmi_gpm.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_mhs_metop-b
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
                          Data/testinput_tier_1/upgrader_mhs_metop-b.nc4
                          testoutput/upgrader_mhs_metop-b.nc4"
                          upgrader_mhs_metop-b.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_sondes
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
                          Data/testinput_tier_1/upgrader_sondes.nc4
                          testoutput/upgrader_sondes.nc4"
                          upgrader_sondes.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_aod
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
                          Data/testinput_tier_1/upgrader_aod.nc4
                          testoutput/upgrader_aod.nc4"
                          upgrader_aod.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_gnssro
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
                          Data/testinput_tier_1/upgrader_gnssro.nc4
                          testoutput/upgrader_gnssro.nc4"
                          upgrader_gnssro.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_met_office_gpsro
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
                          Data/testinput_tier_1/upgrader_met_office_gpsro.nc4
                          testoutput/upgrader_met_office_gpsro.nc4"
                          upgrader_met_office_gpsro.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_zero_obs
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v1-to-v2.x
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

#####################################################################
# Test the ioda-upgrade-v2-to-v3.x app
#####################################################################

# test converting offset date time to epoch datetime
ecbuild_add_test( TARGET  test_ioda_upgrader_dtime_offset_v2_to_v3
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v2-to-v3.x
                          Data/testinput_tier_1/upgrader_dtime_offset_v2_to_v3.nc4
                          testoutput/upgrader_dtime_offset_v2_to_v3.nc4
                          ../share/test/testinput/validation/ObsSpace.yaml"
                          upgrader_dtime_offset_v2_to_v3.nc4
                  TEST_DEPENDS get_ioda_test_data )

# test converting string date time to epoch datetime
# when both offset and string exist in the input file
ecbuild_add_test( TARGET  test_ioda_upgrader_dtime_offstr_v2_to_v3
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v2-to-v3.x
                          Data/testinput_tier_1/upgrader_dtime_offstr_v2_to_v3.nc4
                          testoutput/upgrader_dtime_offstr_v2_to_v3.nc4
                          ../share/test/testinput/validation/ObsSpace.yaml"
                          upgrader_dtime_offstr_v2_to_v3.nc4
                  TEST_DEPENDS get_ioda_test_data )

# test using epoch date time to epoch datetime
# when all of offset, string and epoch exist in the input file
ecbuild_add_test( TARGET  test_ioda_upgrader_dtime_all_v2_to_v3
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v2-to-v3.x
                          Data/testinput_tier_1/upgrader_dtime_all_v2_to_v3.nc4
                          testoutput/upgrader_dtime_all_v2_to_v3.nc4
                          ../share/test/testinput/validation/ObsSpace.yaml"
                          upgrader_dtime_all_v2_to_v3.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_amsua_n19_v2_to_v3
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v2-to-v3.x
                          Data/testinput_tier_1/upgrader_amsua_n19_v2_to_v3.nc4
                          testoutput/upgrader_amsua_n19_v2_to_v3.nc4
                          ../share/test/testinput/validation/ObsSpace.yaml"
                          upgrader_amsua_n19_v2_to_v3.nc4
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_upgrader_met_office_gpsro_v2_to_v3
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/ioda-upgrade-v2-to-v3.x
                          Data/testinput_tier_1/upgrader_met_office_gpsro_v2_to_v3.nc4
                          testoutput/upgrader_met_office_gpsro_v2_to_v3.nc4
                          ../share/test/testinput/validation/ObsSpace.yaml"
                          upgrader_met_office_gpsro_v2_to_v3.nc4
                  TEST_DEPENDS get_ioda_test_data )

if (odc_FOUND)
  ecbuild_add_test( TARGET  test_ioda-convert_aircraft_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Aircraft
                            Data/testinput_tier_1/aircraft.odb
                            ../share/test/testinput/odb_aircraft_name_map.yaml ../share/test/testinput/iodatest_odb_aircraft.yaml"
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
                            ../share/test/testinput/odb_aircraft_name_map.yaml ../share/test/testinput/iodatest_odb_aircraft.yaml"
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
                            ../share/test/testinput/odb_surface_name_map.yaml ../share/test/testinput/iodatest_odb_surface.yaml"
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
                            ../share/test/testinput/odb_surface_name_map.yaml ../share/test/testinput/iodatest_odb_surface.yaml"
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
                            ../share/test/testinput/odb_surface_name_map.yaml testinput/iodatest_odb_surface_with_query.yaml"
                            test-Surface-with-query.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surface_missing_odc
                    TYPE    SCRIPT
                    COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                    ARGS    Surface-missing
                            Data/testinput_tier_1/surface-missing.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_surface.yaml
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_surface_empty_odc
                    TYPE    SCRIPT
                    COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                    ARGS    Surface-empty
                            Data/testinput_tier_1/surface-empty.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_surface.yaml
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde
                            Data/testinput_tier_1/sonde.odb
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde.yaml"
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
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde.yaml"
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
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde_truncate_profiles.yaml"
                            test-Sonde-truncate-profiles.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_history_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde-history
                            Data/testinput_tier_1/sonde.odb
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde_history.yaml"
                            test-Sonde-history.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_time_displacement_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde-time-displacement
                            Data/testinput_tier_1/sonde.odb
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde_time_displacement.yaml"
                            test-Sonde-time-displacement.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_time_displacement_odc_extended_time_window_equal_to_window_start
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde-time-displacement-extended-time-window-equal-to-window-start
                            Data/testinput_tier_1/sonde.odb
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde_time_displacement.yaml
                            2018-04-14T21:00:00Z 2018-04-14T21:00:00Z"
                            test-Sonde-time-displacement-extended-time-window-equal-to-window-start.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_sonde_time_displacement_odc_extended_time_window_before_window_start
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde-time-displacement-extended-time-window-before-window-start
                            Data/testinput_tier_1/sonde.odb
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde_time_displacement.yaml
                            2018-04-14T23:00:00Z 2018-04-14T21:00:00Z"
                            test-Sonde-time-displacement-extended-time-window-before-window-start.hdf
                    TEST_DEPENDS get_ioda_test_data )

  # This test is designed to throw an exception.
  # The exception occurs because the extended lower bound of the time window has been set to
  # a value later than the start of the window.
  # The ioda_compare.sh script checks than an error has occurred and reports success if so.
  # An output file is not produced so the test reference file is just a placeholder.
  ecbuild_add_test( TARGET  test_ioda-convert_sonde_time_displacement_odc_extended_time_window_invalid_datetime
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            Sonde-time-displacement-extended-time-window-invalid-datetime
                            Data/testinput_tier_1/sonde.odb
                            ../share/test/testinput/odb_sonde_name_map.yaml ../share/test/testinput/iodatest_odb_sonde_time_displacement.yaml
                            2018-04-14T21:00:00Z 2018-04-14T23:00:00Z"
                            test-Sonde-time-displacement-extended-time-window-invalid-datetime.hdf
                            0.0
                            N
                            Y
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_abiclr_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            ABIClr
                            Data/testinput_tier_1/abiclr.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_abiclr.yaml"
                            test-ABIClr.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_ahiclr_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            AHIClr
                            Data/testinput_tier_1/ahiclr.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_ahiclr.yaml"
                            test-AHIClr.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_amsr_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            AMSR
                            Data/testinput_tier_1/amsr.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_amsr.yaml"
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
                            ../share/test/testinput/odb_radiance_name_map.yaml ../share/test/testinput/iodatest_odb_airs.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_atovs.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_gmihigh.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_gmilow.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_ssmis.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_satwind.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_scatwind.yaml"
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
                            ../share/test/testinput/odb_gnssro_name_map.yaml ../share/test/testinput/iodatest_odb_gnssro.yaml"
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
                            ../share/test/testinput/odb_gnssro_profile_name_map.yaml testinput/iodatest_odb_gnssro_profile.yaml 333"
                            test-GNSSRO-profile.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_groundgps_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            GroundGPS
                            Data/testinput_tier_1/groundgps.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_groundgps.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_aod.yaml"
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
                            ../share/test/testinput/odb_radiance_name_map.yaml ../share/test/testinput/iodatest_odb_cris.yaml"
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
                            ../share/test/testinput/odb_radiance_name_map.yaml ../share/test/testinput/iodatest_odb_crisfsr.yaml"
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
                            ../share/test/testinput/odb_radiance_name_map.yaml  ../share/test/testinput/iodatest_odb_hirasfsr.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_hloswind.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_mwri.yaml"
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
                            ../share/test/testinput/odb_radiance_name_map.yaml ../share/test/testinput/iodatest_odb_iasi.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_mwsfy3.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_sattcwv.yaml"
                            test-SatTCWV.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_seviriclr_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            SEVIRIClr
                            Data/testinput_tier_1/seviriclr.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_seviriclr.yaml"
                            test-SEVIRIClr.hdf
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda-convert_atms_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            ATMS
                            Data/testinput_tier_1/atms.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_atms.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_geocloud.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_surfacecloud.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_altimeter.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_oceancolour.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_satsst.yaml"
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
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_seaice.yaml"
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
                            ../share/test/testinput/odb_surfacesst_name_map.yaml ../share/test/testinput/iodatest_odb_surfacesst.yaml"
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
                            ../share/test/testinput/odb_oceansound_name_map.yaml ../share/test/testinput/iodatest_odb_oceansound.yaml"
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
                            ../share/test/testinput/odb_oceansound_name_map.yaml ../share/test/testinput/iodatest_odb_oceansound_truncate_profiles.yaml"
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
                  ARGS    "testinput/iodatest_obsdtype.yaml"
                  LIBS    ioda_test )

ecbuild_add_test( TARGET  test_ioda_fileformat
                  SOURCES mains/TestIodaFileFormat.cc
                  ARGS    "testinput/iodatest_fileformat.yaml"
                  LIBS    ioda_test )

#####################################################################
# ioda-validate code coverage test
#####################################################################
# Actual file tests are in ufo-data and in ioda-converters.
# - Future: IODA_DATA_TEST_ROOT is set by the ioda-data project.
ecbuild_add_test(
	TARGET test_ioda-validate
	COMMAND ioda-validate.x
	ARGS "--ignore-warn"
       "--ignore-error"
       "../share/test/testinput/validation/ObsSpace.yaml"
       "Data/testinput_tier_1/sample_hofx_output_amsua_n19.nc4"
	     # Future: "${IODA_DATA_TEST_ROOT}/testinput_tier_1/sample_hofx_output_amsua_n19.nc4"
	)

#####################################################################
# Set up the testinput and testoutput directories
#####################################################################

# Create symlink to input YAML configuration files
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

# Create a work directory for the io pool based reader
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testwork)
