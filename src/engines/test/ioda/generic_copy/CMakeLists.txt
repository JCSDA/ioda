# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-test-generic-copy copy1.cpp)
addapp(ioda-test-generic-copy)
target_link_libraries(ioda-test-generic-copy PUBLIC ioda_engines)

if(0) #BUILD_TESTING)
    add_test(NAME test_ioda-generic-copy-default COMMAND ioda-test-generic-copy)
    add_test(NAME test_ioda-generic-copy-h5file COMMAND ioda-test-generic-copy --ioda-engine-options HDF5-file "fills.hdf5" create truncate)
    add_test(NAME test_ioda-generic-copy-h5mem COMMAND ioda-test-generic-copy --ioda-engine-options HDF5-mem "fills-mem.hdf5" 10 false)
    add_test(NAME test_ioda-generic-copy-ObsStore COMMAND ioda-test-generic-copy --ioda-engine-options obs-store)
endif()

