# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_hdf5 test.cpp)
addapp(ioda-engines_hdf5)
target_link_libraries(ioda-engines_hdf5 PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_hdf5 COMMAND ioda-engines_hdf5)
endif()
