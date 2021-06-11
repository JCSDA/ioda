# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-test-fillvalues ioda-test-fillvalues.cpp)
addapp(ioda-test-fillvalues)
target_link_libraries(ioda-test-fillvalues PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-fillvalues-default COMMAND ioda-test-fillvalues)
    add_test(NAME test_ioda-fillvalues-h5file COMMAND ioda-test-fillvalues --ioda-engine-options HDF5-file "fills.hdf5" create truncate)
    add_test(NAME test_ioda-fillvalues-h5mem COMMAND ioda-test-fillvalues --ioda-engine-options HDF5-mem "fills-mem.hdf5" 10 false)
    add_test(NAME test_ioda-fillvalues-ObsStore COMMAND ioda-test-fillvalues --ioda-engine-options obs-store)
endif()

