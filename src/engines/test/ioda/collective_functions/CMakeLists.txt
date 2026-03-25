# (C) Copyright 2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda_test_collective_functions ioda-test-collective-functions.cpp)
addapp(ioda_test_collective_functions)
target_link_libraries(ioda_test_collective_functions PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME ioda_collective_functions_h5file COMMAND ioda_test_collective_functions --ioda-engine-options HDF5-file "collective.hdf5" create truncate)
    add_test(NAME ioda_collective_functions_h5mem COMMAND ioda_test_collective_functions --ioda-engine-options HDF5-mem "collective-mem.hdf5" 10 false)
endif()

