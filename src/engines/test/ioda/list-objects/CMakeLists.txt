# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_list_objects test.cpp)
addapp(ioda-engines_list_objects)
target_link_libraries(ioda-engines_list_objects PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_list_objects-default COMMAND ioda-engines_list_objects)
    add_test(NAME test_ioda-engines_list_objects-h5file COMMAND ioda-engines_list_objects --ioda-engine-options HDF5-file "list_objects-file.hdf5" create truncate)
    add_test(NAME test_ioda-engines_list_objects-h5mem COMMAND ioda-engines_list_objects --ioda-engine-options HDF5-mem "list_objects-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines_list_objects-ObsStore COMMAND ioda-engines_list_objects --ioda-engine-options obs-store)
endif()
