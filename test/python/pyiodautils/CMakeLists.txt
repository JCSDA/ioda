# Tests for the pyiodautils package
# Python3 has been found so add in tests

# make sure starting with a clean slate
ecbuild_add_test( TARGET  test_ioda_clean_pyiodautils_file_merge_files
                    COMMAND rm
                    ARGS    "-f" "testoutput/sondes_file_merge_concat_out.nc4")

# create a concatenated file
ecbuild_add_test ( TARGET test_ioda_pyiodautils_file_merge
                   COMMAND ${Python3_EXECUTABLE}
                   ARGS ${CMAKE_CURRENT_SOURCE_DIR}/test_file_merge_concat_method.py
                   --outfile testoutput/sondes_file_merge_concat_out.nc4
                   --infiles Data/testinput_tier_1/sondes_file_merge_concat_in_0000.nc4
                             Data/testinput_tier_1/sondes_file_merge_concat_in_0001.nc4
                   ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}:$ENV{PYTHONPATH}
                   TEST_DEPENDS get_ioda_test_data test_ioda_clean_pyiodautils_file_merge_files )

# check the concatenated file contents
ecbuild_add_test( TARGET  test_ioda_pyiodautils_file_merge_cmp
                  TYPE    SCRIPT
                  COMMAND bash
                  ARGS    ${CMAKE_BINARY_DIR}/bin/ioda_compare.sh
                          netcdf
                          "echo Checking file merge output"
                          sondes_file_merge_concat_out.nc4
                  TEST_DEPENDS get_ioda_test_data test_ioda_pyiodautils_file_merge )

# Set up connections to test data
if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Data/testinput_tier_1")
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
  file(CREATE_LINK ${ioda_data_testinput_tier_1_local} ${CMAKE_CURRENT_BINARY_DIR}/Data/testinput_tier_1 SYMBOLIC)
endif()

# Set up directory for test output
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testoutput)
