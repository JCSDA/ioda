# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_persist test.cpp)
addapp(ioda-engines_persist)
target_link_libraries(ioda-engines_persist PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_persist-default COMMAND ioda-engines_persist)
    add_test(NAME test_ioda-engines_persist-h5file COMMAND ioda-engines_persist --ioda-engine-options HDF5-file "IODApersist-file.hdf5" create truncate)
    add_test(NAME test_ioda-engines_persist-h5mem COMMAND ioda-engines_persist --ioda-engine-options HDF5-mem "IODApersist-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines_persist-ObsStore COMMAND ioda-engines_persist --ioda-engine-options obs-store)
endif()
