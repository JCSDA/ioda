# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_obsgroup test.cpp)
addapp(ioda-engines_obsgroup)
target_link_libraries(ioda-engines_obsgroup PUBLIC ioda_engines)

add_executable(ioda-engines_obsgroup_append test_append.cpp)
addapp(ioda-engines_obsgroup_append)
target_link_libraries(ioda-engines_obsgroup_append PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_obsgroup-default COMMAND ioda-engines_obsgroup)
    add_test(NAME test_ioda-engines_obsgroup-h5file COMMAND ioda-engines_obsgroup --ioda-engine-options HDF5-file "ioda-engines_obsgroup-file.hdf5" create truncate)
    add_test(NAME test_ioda-engines_obsgroup-h5mem COMMAND ioda-engines_obsgroup --ioda-engine-options HDF5-mem "ioda-engines_obsgroup-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines_obsgroup-ObsStore COMMAND ioda-engines_obsgroup --ioda-engine-options obs-store)

    add_test(NAME test_ioda-engines_obsgroup_append-h5file COMMAND ioda-engines_obsgroup_append "file")
    add_test(NAME test_ioda-engines_obsgroup_append-mem COMMAND ioda-engines_obsgroup_append "memory")
endif()
