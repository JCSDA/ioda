# (C) Copyright 2020-2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Paths that go into configuration files...
# save path of test directory to get YAML resource files
set(IODA_ENGINES_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/ioda)

add_subdirectory(ioda)
add_subdirectory(test)
add_subdirectory(Examples)

