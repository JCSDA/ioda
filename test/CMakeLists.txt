# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

find_program (NCCMP_EXE nccmp)
if (NOT NCCMP_EXE)
  message (FATAL_ERROR "nccmp needs to be available to run ioda tests.")
endif()

find_program (PYCODESTYLE_EXE pycodestyle)
if (NOT PYCODESTYLE_EXE)
  message (FATAL_ERROR "pycodestyle needs to be available to run ioda tests.")
endif()

# Test data - from ioda-data repo if found, from local directory, or from tarball (release/develop-in-future?)
if (ioda-data_FOUND)
  message(STATUS "Use test data from ioda-data repository")
  set (ioda_data_testinput_tier_1_local "${CMAKE_SOURCE_DIR}/ioda-data/testinput_tier_1")
# It's unclear if anyone is using the local data functionality, therefore comment it out
# for now and add in if someone complains / remove if completely if not.
#elseif (DEFINED ENV{IODA_TESTFILES})
#  message(STATUS "Use test data from local directory $ENV{IODA_TESTFILES}")
#  # A bit of guesswork here, I don't know if folks using this option stored it in the same directory structure
#  set (ioda_data_testinput_tier_1_local "$ENV{IODA_TESTFILES}/ioda-data/testinput_tier_1")
else()
  set (ioda_data_testinput_tier_1_local "${CMAKE_SOURCE_DIR}/test-data-release/ioda/${ioda_data_tag}/testinput_tier_1")
  if (EXISTS ${ioda_data_testinput_tier_1_local})
    message(STATUS "Skipping download of test data from ${ioda_data_download_url}/${ioda_data_tarball_name} since it already exists")
  else()
    message(STATUS "Download test data from ${ioda_data_download_url}/${ioda_data_tarball_name}")
    include(FetchContent)
    FetchContent_Declare(
      ioda-testdata
      DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/test-data-release
      DOWNLOAD_NO_EXTRACT ON
      URL      ${ioda_data_download_url}/${ioda_data_tarball_name}
      URL_HASH MD5=${ioda_data_tarball_hash}
    )
    FetchContent_MakeAvailable(ioda-testdata)
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_SOURCE_DIR}/test-data-release/${ioda_data_tarball_name} DESTINATION ${CMAKE_SOURCE_DIR}/test-data-release)
  endif()
endif()
if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Data/testinput_tier_1")
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
  file(CREATE_LINK ${ioda_data_testinput_tier_1_local} ${CMAKE_CURRENT_BINARY_DIR}/Data/testinput_tier_1 SYMBOLIC)
endif()

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
  testinput/iodatest_obsspace_missing_obs_file_warn.yaml
  testinput/iodatest_obsspace_missing_obs_file_warn_mpi.yaml
  testinput/iodatest_obsspace_missing_obs_file_error.yaml
  testinput/iodatest_obsspace_missing_obs_file_warn_odc.yaml
  testinput/iodatest_obsspace_missing_obs_file_warn_odc_mpi.yaml
  testinput/iodatest_obsspace_missing_obs_file_error_odc.yaml
  testinput/iodatest_obsspace_missing_obs_file_error_odc_reader_pool.yaml
  testinput/iodatest_obsspace_missing_obs_file_error_reader_pool.yaml
  testinput/iodatest_obsspace_out_dims_check.yaml
  testinput/iodatest_obsspace_grouping.yaml
  testinput/iodatest_obsspace_grouping_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum.yaml
  testinput/iodatest_obsspace_index_recnum_twfilt.yaml
  testinput/iodatest_obsspace_index_recnum_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum_mpi_2_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum_mpi_4_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum_twfilt_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum_twfilt_mpi_2_reader_pool.yaml
  testinput/iodatest_obsspace_index_recnum_twfilt_mpi_4_reader_pool.yaml
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
  testinput/iodatest_obsspace_out_odc_sonde_mixed.yaml
  testinput/iodatest_obsspace_out_odc_readbackin.yaml
  testinput/iodatest_obsspace_out_odc_mpi_2.yaml
  testinput/iodatest_obsspace_put_db_channels.yaml
  testinput/iodatest_obsspace_put_db_channels_check.yaml
  testinput/iodatest_obsspace_reduce.yaml
  testinput/iodatest_obsspace_reduce_mpi.yaml
  testinput/iodatest_obsspace_zero_obs.yaml
  testinput/iodatest_obsspace_filter_to_zero_obs.yaml
  testinput/iodatest_obsspace_zero_obs_reader_pool.yaml
  testinput/iodatest_obsspace_filter_to_zero_obs_reader_pool.yaml
  testinput/iodatest_obsspace_fill_value.yaml
  testinput/iodatest_obsspace_inclusive_lower_bound.yaml
  testinput/iodatest_obsvector.yaml
  testinput/iodatest_obsvector_grouped.yaml
  testinput/iodatest_obsvector_missing_locs.yaml
  testinput/iodatest_obsvector_packeigen.yaml
  testinput/iodatest_extendedobsspace.yaml
  testinput/iodatest_extendedobsspace_halo.yaml
  testinput/iodatest_odb_surface_with_query.yaml
  testinput/iodatest_sort.yaml
  testinput/iodatest_writer_sondes.yaml
  testinput/iodatest_build_amsua_n19_file_set.yaml
  testinput/iodatest_build_sondes_file_set.yaml
  testinput/iodatest_build_gnssro_file_set.yaml
  testinput/iodatest_build_amsua_n19_file_set_mpi7.yaml
  testinput/iodatest_build_sondes_file_set_mpi7.yaml
  testinput/iodatest_build_gnssro_file_set_mpi7.yaml
  testinput/iodatest_time_io.yaml
  testinput/iodatest_time_io_odc.yaml
  testinput/iodatest_time_io_reader_pool.yaml
  testinput/iodatest_time_io_reader_pool_odc.yaml
  testinput/iodatest_time_io_reader_pool_ext_file_prep.yaml
  testinput/iodatest_time_io_reader_pool_ext_file_prep_mpi7.yaml
  testinput/iodatest_time_io_script.yaml
  testinput/iodatest_time_io_script.py
)

# Work directories for the new reader. Make one entry per test that is using
# the work directory. This is done so that two tests running at the same time
# (ie, running ctest -j8) do not collide.
#
# TODO(SRH) Need to add a macro that will define these along with the test name
# and corresponding yaml config file.
list( APPEND ioda_test_work_directories
  testwork/obsspace_empty_obs_file_mpi_reader_pool
  testwork/obsspace_empty_obs_file_reader_pool
  testwork/obsspace_filter_to_zero_obs_reader_pool
  testwork/obsspace_grouping_reader_pool
  testwork/obsspace_index_recnum_reader_pool
  testwork/obsspace_index_recnum_mpi_2_reader_pool
  testwork/obsspace_index_recnum_mpi_4_reader_pool
  testwork/obsspace_index_recnum_twfilt_reader_pool
  testwork/obsspace_index_recnum_twfilt_mpi_2_reader_pool
  testwork/obsspace_index_recnum_twfilt_mpi_4_reader_pool
  testwork/obsspace_missing_obs_file_error_odc_reader_pool
  testwork/obsspace_missing_obs_file_error_reader_pool
  testwork/obsspace_missing_obs_file_warn
  testwork/obsspace_missing_obs_file_warn_mpi
  testwork/obsspace_missing_obs_file_warn_odc
  testwork/obsspace_missing_obs_file_warn_odc_mpi
  testwork/obsspace_mpi_reader_pool
  testwork/obsspace_odc_atms_reader_pool
  testwork/obsspace_odc_reader_pool
  testwork/obsspace_reader_pool
  testwork/obsspace_zero_obs_reader_pool
  testwork/time_io_reader_pool
  testwork/time_io_reader_pool_odc
  testwork/build_file_set
  testwork/build_file_set_mpi7
)

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
                    ARGS    "-f" "testoutput/diagout-aircraft.odc" "testoutput/diagout-atms.odc" "testoutput/diagout-gmi.odc"
                                 "testoutput/diagout-amsua.odc" "testoutput/diagout-sonde.odc" "testoutput/diagout-sonde-mixed.odc" "testoutput/diagout-aod.odc"
                                 "testoutput/diagout-iasi.odc" "testoutput/diagout-aod-multifiles_0000.odc"
                                 "testoutput/diagout-aod-multifiles_0001.odc" "testoutput/diagout-aod-multifiles-oneiopool.odc"
                                 "testoutput/diagout-aod-singlefile-oneiopool.odc" "testoutput/diagout-aod-singlefile.odc" "testoutput/diagout-atms-readbackin.odc"
                                 "testoutput/diagout-aircraft-readbackin.odc" "testoutput/diagout-atms-readbackin.odc" "testoutput/diagout-iasi-readbackin.odc")
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_out_odc.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS get_ioda_test_data test_ioda_clean_obsspace_out_odc_files )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_sonde_mixed
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_out_odc_sonde_mixed.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS get_ioda_test_data test_ioda_clean_obsspace_out_odc_files )
  ecbuild_add_test( TARGET  iodatest_obsspace_out_odc_readbackin
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_out_odc_readbackin.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS test_ioda_obsspace_out_odc get_ioda_test_data test_ioda_obsspace_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_mpi_2
                    MPI 2
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_out_odc_mpi_2.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS get_ioda_test_data test_ioda_clean_obsspace_out_odc_files )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aircraft
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aircraft.odc" "Data/testinput_tier_1/test_reference/diagout-aircraft.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_atms
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-atms.odc" "Data/testinput_tier_1/test_reference/diagout-atms.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_gmi
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-gmi.odc" "Data/testinput_tier_1/test_reference/diagout-gmi.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_amsua
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-amsua.odc" "Data/testinput_tier_1/test_reference/diagout-amsua.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_sonde
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-sonde.odc" "Data/testinput_tier_1/test_reference/diagout-sonde.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_sonde_mixed
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-sonde-mixed.odc" "Data/testinput_tier_1/test_reference/diagout-sonde-mixed.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_sonde_mixed )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_sonde_derived
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-sonde-derived.odc" "Data/testinput_tier_1/test_reference/diagout-sonde-derived.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_sonde_mixed )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod.odc" "Data/testinput_tier_1/test_reference/diagout-aod.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_iasi
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-iasi.odc" "Data/testinput_tier_1/test_reference/diagout-iasi.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod_multifiles_0000
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod-multifiles_0000.odc" "Data/testinput_tier_1/test_reference/diagout-aod-multifiles_0000.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_mpi_2 )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod_multifiles_0001
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod-multifiles_0001.odc" "Data/testinput_tier_1/test_reference/diagout-aod-multifiles_0001.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_mpi_2 )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod_multifiles_oneiopool
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod-multifiles-oneiopool.odc" "Data/testinput_tier_1/test_reference/diagout-aod-2mpi.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_mpi_2 )

  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod_singlefile
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod-singlefile.odc" "Data/testinput_tier_1/test_reference/diagout-aod-2mpi.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_mpi_2 )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_aod_singlefile_oneiopool
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aod-singlefile-oneiopool.odc" "Data/testinput_tier_1/test_reference/diagout-aod-2mpi.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_out_odc_mpi_2 )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_readbackin_aircraft
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-aircraft-readbackin.odc" "Data/testinput_tier_1/test_reference/diagout-aircraft-readbackin.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data iodatest_obsspace_out_odc_readbackin )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_readbackin_atms
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-atms-readbackin.odc" "Data/testinput_tier_1/test_reference/diagout-atms-readbackin.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data iodatest_obsspace_out_odc_readbackin )
  ecbuild_add_test( TARGET  test_ioda_obsspace_out_odc_compare_readbackin_iasi
                    COMMAND ${CMAKE_BINARY_DIR}/bin/compare-odbs
                    ARGS    "testoutput/diagout-iasi-readbackin.odc" "Data/testinput_tier_1/test_reference/diagout-iasi-readbackin.odc" ${ODC_TOOL_PATH}
                    TEST_DEPENDS get_ioda_test_data iodatest_obsspace_out_odc_readbackin )
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

ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_warn
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_missing_obs_file_warn.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_warn_mpi
                  MPI 4
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_missing_obs_file_warn_mpi.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

# The next two tests are verifying that an exception is thrown (ie, error) when the input
# obs file is missing which the "missing file action: error" YAML spec is used.
# This is accomplished by running the ioda_compare.sh script and telling that script
# that an error return code (non-zero) is expected. The arguments to the ioda_compare.sh
# script are:
#      1 - type of file to compare (only hdf5 will check for error return code)
#      2 - command to run (time_IodaIO.x with its single YAML file argument)
#      3 - file to test
#      4 - tolerance for comparing floating point values
#      5 - verbosity (Y/N)
#      6 - expect an error (Y/N)

# old reader
ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_error
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                          testinput/iodatest_obsspace_missing_obs_file_error.yaml"
                          testoutput/ospace.missing_obs_file_error.nc4
                          0.0
                          N
                          Y
                  TEST_DEPENDS get_ioda_test_data )

# new reader
ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_error_reader_pool
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          hdf5
                          "${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                          testinput/iodatest_obsspace_missing_obs_file_error_reader_pool.yaml"
                          testoutput/ospace.missing_obs_file_error_reader_pool.nc4
                          0.0
                          N
                          Y
                  TEST_DEPENDS get_ioda_test_data )

if (odc_FOUND)
  ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_warn_odc
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_missing_obs_file_warn_odc.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS get_ioda_test_data )

  ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_warn_odc_mpi
                    MPI 4
                    COMMAND test_ioda_obsspace
                    ARGS    "testinput/iodatest_obsspace_missing_obs_file_warn_odc_mpi.yaml"
                    LIBS  ioda_test
                    TEST_DEPENDS get_ioda_test_data )

  # The next two tests are verifying that an exception is thrown (ie, error) when the input
  # obs file is missing which the "missing file action: error" YAML spec is used.
  # This is accomplished by running the ioda_compare.sh script and telling that script
  # that an error return code (non-zero) is expected. The arguments to the ioda_compare.sh
  # script are:
  #      1 - type of file to compare (only hdf5 will check for error return code)
  #      2 - command to run (time_IodaIO.x with its single YAML file argument)
  #      3 - file to test
  #      4 - tolerance for comparing floating point values
  #      5 - verbosity (Y/N)
  #      6 - expect an error (Y/N)

  # old reader
  ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_error_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                            testinput/iodatest_obsspace_missing_obs_file_error_odc.yaml"
                            testoutput/ospace.missing_obs_file_error_odc.nc4
                            0.0
                            N
                            Y
                    TEST_DEPENDS get_ioda_test_data )

  # old reader
  ecbuild_add_test( TARGET  test_ioda_obsspace_missing_obs_file_error_odc_reader_pool
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                            testinput/iodatest_obsspace_missing_obs_file_error_odc_reader_pool.yaml"
                            testoutput/ospace.missing_obs_file_error_odc_reader_pool.nc4
                            0.0
                            N
                            Y
                    TEST_DEPENDS get_ioda_test_data )
endif()

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
                  ARGS    "testinput/iodatest_obsspace_index_recnum_mpi_2_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_mpi_4_reader_pool
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_mpi_4_reader_pool.yaml"
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
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt_mpi_2_reader_pool.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data )

ecbuild_add_test( TARGET  test_ioda_obsspace_index_recnum_twfilt_mpi_4_reader_pool
                  MPI 4
                  COMMAND test_ioda_obsspace_index_recnum
                  ARGS    "testinput/iodatest_obsspace_index_recnum_twfilt_mpi_4_reader_pool.yaml"
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

###################################################################
##### The following tests check the ObsSpace::reduce function. ####
###################################################################

# this test removes the output files from prior testing
ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_clean_files
                  COMMAND rm
                  ARGS    "-f"
                          "testoutput/obsspace_reduce_equal_1D_out.nc4"
                          "testoutput/obsspace_reduce_not_equal_1D_out.nc4"
                          "testoutput/obsspace_reduce_greater_than_1D_out.nc4"
                          "testoutput/obsspace_reduce_less_than_1D_out.nc4"
                          "testoutput/obsspace_reduce_greater_than_or_equal_1D_out.nc4"
                          "testoutput/obsspace_reduce_less_than_or_equal_1D_out.nc4"
                          "testoutput/obsspace_reduce_equal_2D_out.nc4"
                          "testoutput/obsspace_reduce_not_equal_2D_out.nc4"
                          "testoutput/obsspace_reduce_greater_than_2D_out.nc4"
                          "testoutput/obsspace_reduce_less_than_2D_out.nc4"
                          "testoutput/obsspace_reduce_greater_than_or_equal_2D_out.nc4"
                          "testoutput/obsspace_reduce_less_than_or_equal_2D_out.nc4")

# this test creates the output files, and checks that obs space data members got
# properly updated
ecbuild_add_test( TARGET  test_ioda_obsspace_reduce
                  SOURCES mains/TestIodaObsSpaceReduce.cc
                  ARGS    "testinput/iodatest_obsspace_reduce.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce_clean_files )

# the following tests compare the output files
ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_equal_1D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce equal, 1D variables"
                          obsspace_reduce_equal_1D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_not_equal_1D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce equal, 1D variables"
                          obsspace_reduce_not_equal_1D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_greater_than_1D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce greater than, 1D variables"
                          obsspace_reduce_greater_than_1D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_less_than_1D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce less than, 1D variables"
                          obsspace_reduce_less_than_1D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_greater_than_or_equal_1D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce greater than_or_equal, 1D variables"
                          obsspace_reduce_greater_than_or_equal_1D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_less_than_or_equal_1D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce less than_or_equal, 1D variables"
                          obsspace_reduce_less_than_or_equal_1D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_equal_2D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce equal, 2D variables"
                          obsspace_reduce_equal_2D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_not_equal_2D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce equal, 2D variables"
                          obsspace_reduce_not_equal_2D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_greater_than_2D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce greater than, 2D variables"
                          obsspace_reduce_greater_than_2D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_less_than_2D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce less than, 2D variables"
                          obsspace_reduce_less_than_2D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_greater_than_or_equal_2D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce greater than_or_equal, 2D variables"
                          obsspace_reduce_greater_than_or_equal_2D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_less_than_or_equal_2D_variables_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking reduce less than_or_equal, 2D variables"
                          obsspace_reduce_less_than_or_equal_2D_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

# The next two tests run with 2 and 3 MPI tasks repectively using the same input files
# as test_ioda_obssspace_reduce and only checks the obs space data members (no output files).
ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_mpi_2
                  MPI 2
                  COMMAND test_ioda_obsspace_reduce
                  ARGS    "testinput/iodatest_obsspace_reduce_mpi.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

ecbuild_add_test( TARGET  test_ioda_obsspace_reduce_mpi_3
                  MPI 3
                  COMMAND test_ioda_obsspace_reduce
                  ARGS    "testinput/iodatest_obsspace_reduce_mpi.yaml"
                  LIBS  ioda_test
                  TEST_DEPENDS get_ioda_test_data test_ioda_obsspace_reduce)

###################################################################
################ End of reduce function tests #####################
###################################################################

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

ecbuild_add_test( TARGET  test_ioda_obsspace_inclusive_lower_bound
                  COMMAND test_ioda_obsspace
                  ARGS    "testinput/iodatest_obsspace_inclusive_lower_bound.yaml"
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
                  ARGS    "testinput/iodatest_obsvector_grouped.yaml"
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
                          "testoutput/gnssro_obs_2021080606_s_time_reader_pool.nc4"
                          "testoutput/amsua_n19_obs_2018041500_m_time_reader_pool_ext_file_prep.nc4"
                          "testoutput/sondes_obs_2018041500_m_time_reader_pool_ext_file_prep.nc4"
                          "testoutput/gnssro_obs_2021080606_s_time_reader_pool_ext_file_prep.nc4"
                          "testoutput/amsua_n19_obs_2018041500_m_time_reader_pool_ext_file_prep_mpi7.nc4"
                          "testoutput/sondes_obs_2018041500_m_time_reader_pool_ext_file_prep_mpi7.nc4"
                          "testoutput/gnssro_obs_2021080606_s_time_reader_pool_ext_file_prep_mpi7.nc4" )

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
                          netcdf
                          "echo Checking time io sondes"
                          sondes_obs_2018041500_m_time.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_amsua_n19
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io AMSU-A NOAA19"
                          amsua_n19_obs_2018041500_m_time.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_gnssro
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
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
                          netcdf
                          "echo Checking time io (reader pool) sondes"
                          sondes_obs_2018041500_m_time_reader_pool.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_amsua_n19_reader_pool
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io (reader pool) AMSU-A NOAA19"
                          amsua_n19_obs_2018041500_m_time_reader_pool.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_gnssro_reader_pool
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
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
                            aircraft_time.odb
                    TEST_DEPENDS get_ioda_test_data test_ioda_time_io_odc)

  ecbuild_add_test( TARGET  test_ioda_time_io_cmp_atms_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            odb
                            "echo Checking time io ATMS"
                            atms_time.odb
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
                            aircraft_time_reader_pool.odb
                    TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_odc)

  ecbuild_add_test( TARGET  test_ioda_time_io_cmp_atms_reader_pool_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            odb
                            "echo Checking time io (reader pool) ATMS"
                            atms_time_reader_pool.odb
                    TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_odc)
endif()

#####################################################################
# Test the new reader with externally prepared input files
#####################################################################

# this test creates new output files
ecbuild_add_test( TARGET  test_ioda_time_io_reader_pool_ext_file_prep
                  COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                  ARGS    "testinput/iodatest_time_io_reader_pool_ext_file_prep.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_time_io_files test_ioda_build_amsua_n19_file_set test_ioda_build_gnssro_file_set test_ioda_build_sondes_file_set )

# the following tests compare the output files
ecbuild_add_test( TARGET  test_ioda_time_io_cmp_sondes_reader_pool_ext_file_prep
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io (reader pool ext file prep) sondes"
                          sondes_obs_2018041500_m_time_reader_pool_ext_file_prep.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_ext_file_prep )

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_amsua_n19_reader_pool_ext_file_prep
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io (reader pool ext file prep) AMSU-A NOAA19"
                          amsua_n19_obs_2018041500_m_time_reader_pool_ext_file_prep.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_ext_file_prep )

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_gnssro_reader_pool_ext_file_prep
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io (reader pool ext file prep) GNSSRO"
                          gnssro_obs_2021080606_s_time_reader_pool_ext_file_prep.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_ext_file_prep)

# this test creates new output files
ecbuild_add_test( TARGET  test_ioda_time_io_reader_pool_ext_file_prep_mpi7
                  MPI 7
                  COMMAND ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                  ARGS    "testinput/iodatest_time_io_reader_pool_ext_file_prep_mpi7.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_time_io_files test_ioda_build_amsua_n19_file_set_mpi7 test_ioda_build_gnssro_file_set_mpi7 test_ioda_build_sondes_file_set_mpi7 )

# the following tests compare the output files
ecbuild_add_test( TARGET  test_ioda_time_io_cmp_sondes_reader_pool_ext_file_prep_mpi7
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io (reader pool ext file prep) sondes"
                          sondes_obs_2018041500_m_time_reader_pool_ext_file_prep_mpi7.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_ext_file_prep_mpi7)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_amsua_n19_reader_pool_ext_file_prep_mpi7
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io (reader pool ext file prep) AMSU-A NOAA19"
                          amsua_n19_obs_2018041500_m_time_reader_pool_ext_file_prep_mpi7.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_ext_file_prep_mpi7)

ecbuild_add_test( TARGET  test_ioda_time_io_cmp_gnssro_reader_pool_ext_file_prep_mpi7
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking time io (reader pool ext file prep) GNSSRO"
                          gnssro_obs_2021080606_s_time_reader_pool_ext_file_prep_mpi7.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_time_io_reader_pool_ext_file_prep_mpi7)

#####################################################################
# Test the ioda-buildInputFileSet.x app
#####################################################################

# this test removes the output files from prior testing
ecbuild_add_test( TARGET  test_ioda_clean_build_file_set_files
                  COMMAND rm
                  ARGS    "-f"
                    "testwork/build_file_set/amsua_n19_obs_2018041500_m_prep_file_info.nc4"
                    "testwork/build_file_set/amsua_n19_obs_2018041500_m_0000.nc4"
                    "testwork/build_file_set/sondes_obs_2018041500_m_prep_file_info.nc4"
                    "testwork/build_file_set/sondes_obs_2018041500_m_0000.nc4"
                    "testwork/build_file_set/gnssro_obs_2021080606_s_prep_file_info.nc4"
                    "testwork/build_file_set/gnssro_obs_2021080606_s_0000.nc4"
                    "testwork/build_file_set_mpi7/amsua_n19_obs_2018041500_m_prep_file_info_mpi7.nc4"
                    "testwork/build_file_set_mpi7/amsua_n19_obs_2018041500_m_mpi7_0000.nc4"
                    "testwork/build_file_set_mpi7/amsua_n19_obs_2018041500_m_mpi7_0001.nc4"
                    "testwork/build_file_set_mpi7/amsua_n19_obs_2018041500_m_mpi7_0002.nc4"
                    "testwork/build_file_set_mpi7/amsua_n19_obs_2018041500_m_mpi7_0003.nc4"
                    "testwork/build_file_set_mpi7/sondes_obs_2018041500_m_prep_file_info_mpi7.nc4"
                    "testwork/build_file_set_mpi7/sondes_obs_2018041500_m_mpi7_0000.nc4"
                    "testwork/build_file_set_mpi7/sondes_obs_2018041500_m_mpi7_0001.nc4"
                    "testwork/build_file_set_mpi7/sondes_obs_2018041500_m_mpi7_0002.nc4"
                    "testwork/build_file_set_mpi7/sondes_obs_2018041500_m_mpi7_0003.nc4"
                    "testwork/build_file_set_mpi7/gnssro_obs_2021080606_s_prep_file_info_mpi7.nc4"
                    "testwork/build_file_set_mpi7/gnssro_obs_2021080606_s_mpi7_0000.nc4"
                    "testwork/build_file_set_mpi7/gnssro_obs_2021080606_s_mpi7_0001.nc4"
                    "testwork/build_file_set_mpi7/gnssro_obs_2021080606_s_mpi7_0002.nc4"
                    "testwork/build_file_set_mpi7/gnssro_obs_2021080606_s_mpi7_0003.nc4" )

# amsua n19, communicator target size 1
ecbuild_add_test( TARGET  test_ioda_build_amsua_n19_file_set
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-buildInputFileSet.x
                  ARGS    "testinput/iodatest_build_amsua_n19_file_set.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_build_file_set_files )

# sondes, communicator target size 1
ecbuild_add_test( TARGET  test_ioda_build_sondes_file_set
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-buildInputFileSet.x
                  ARGS    "testinput/iodatest_build_sondes_file_set.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_build_file_set_files )

# gnssro, communicator target size 1
ecbuild_add_test( TARGET  test_ioda_build_gnssro_file_set
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-buildInputFileSet.x
                  ARGS    "testinput/iodatest_build_gnssro_file_set.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_build_file_set_files )

# amsua n19, communicator target size 7
ecbuild_add_test( TARGET  test_ioda_build_amsua_n19_file_set_mpi7
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-buildInputFileSet.x
                  ARGS    "testinput/iodatest_build_amsua_n19_file_set_mpi7.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_build_file_set_files )

# sondes, communicator target size 7
ecbuild_add_test( TARGET  test_ioda_build_sondes_file_set_mpi7
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-buildInputFileSet.x
                  ARGS    "testinput/iodatest_build_sondes_file_set_mpi7.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_build_file_set_files )

# gnssro, communicator target size 7
ecbuild_add_test( TARGET  test_ioda_build_gnssro_file_set_mpi7
                  COMMAND ${CMAKE_BINARY_DIR}/bin/ioda-buildInputFileSet.x
                  ARGS    "testinput/iodatest_build_gnssro_file_set_mpi7.yaml"
                  TEST_DEPENDS get_ioda_test_data test_ioda_clean_build_file_set_files )

#####################################################################
# Test the script backend
#####################################################################

if (BUILD_PYTHON_BINDINGS)
  ecbuild_add_test( TARGET      test_ioda_time_io_script
                    COMMAND     ${CMAKE_BINARY_DIR}/bin/time_IodaIO.x
                    ARGS        "testinput/iodatest_time_io_script.yaml"
                    ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}:$ENV{PYTHONPATH})
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
  ecbuild_add_test( TARGET  test_ioda-convert_amsub_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            AMSUB
                            Data/testinput_tier_1/amsub.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_amsub.yaml"
                            test-AMSUB.hdf
                    TEST_DEPENDS get_ioda_test_data )
  ecbuild_add_test( TARGET  test_ioda-convert_ascatsm_odc
                    TYPE    SCRIPT
                    COMMAND bash
                    ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                            hdf5
                            "${CMAKE_BINARY_DIR}/bin/ioda-odc-converter.x
                            ASCATSM
                            Data/testinput_tier_1/ascatsm.odb
                            ../share/test/testinput/odb_default_name_map.yaml ../share/test/testinput/iodatest_odb_ascatsm.yaml"
                            test-ASCATSM.hdf
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
                            ../share/test/testinput/odb_gnssro_profile_name_map.yaml ../share/test/testinput/iodatest_odb_gnssro_profile.yaml 333"
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

# Create work directories for the io pool based reader
foreach(DIRNAME ${ioda_test_work_directories})
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${DIRNAME})
endforeach()

#####################################################################
# Set up testing of python utilities
#####################################################################
if (Python3_FOUND)
    add_subdirectory( python )
endif()
