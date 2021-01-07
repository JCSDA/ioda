# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-test-chunks_and_filters ioda-test-chunks_and_filters.cpp)
addapp(ioda-test-chunks_and_filters)
target_link_libraries(ioda-test-chunks_and_filters PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-chunks_and_filters-default COMMAND ioda-test-chunks_and_filters)
    add_test(NAME test_ioda-chunks_and_filters-h5file COMMAND ioda-test-chunks_and_filters --ioda-engine-options HDF5-file "test-filters-file.hdf5" create truncate)
    add_test(NAME test_ioda-chunks_and_filters-h5mem COMMAND ioda-test-chunks_and_filters --ioda-engine-options HDF5-mem "test-filters-mem.hdf5" 10 false)
    add_test(NAME test_ioda-chunks_and_filters-ObsStore COMMAND ioda-test-chunks_and_filters --ioda-engine-options obs-store)
endif()

