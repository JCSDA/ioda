# (C) Copyright 2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-test-collective-functions ioda-test-collective-functions.cpp)
addapp(ioda-test-collective-functions)
target_link_libraries(ioda-test-collective-functions PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-collective-functions-h5file COMMAND ioda-test-collective-functions --ioda-engine-options HDF5-file "collective.hdf5" create truncate)
    add_test(NAME test_ioda-collective-functions-h5mem COMMAND ioda-test-collective-functions --ioda-engine-options HDF5-mem "collective-mem.hdf5" 10 false)
endif()

