# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_data-selections test.cpp)
addapp(ioda-engines_data-selections)
target_link_libraries(ioda-engines_data-selections PUBLIC ioda_engines)

add_executable(ioda-engines_dim-selectors test_dim_selectors.cpp)
addapp(ioda-engines_dim-selectors)
target_link_libraries(ioda-engines_dim-selectors PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_data-selections-default COMMAND ioda-engines_data-selections)
    add_test(NAME test_ioda-engines_data-selections-h5file COMMAND ioda-engines_data-selections --ioda-engine-options HDF5-file "data-selections-file.hdf5" create truncate)
    add_test(NAME test_ioda-engines_data-selections-h5mem COMMAND ioda-engines_data-selections --ioda-engine-options HDF5-mem "data-selections-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines_data-selections-ObsStore COMMAND ioda-engines_data-selections --ioda-engine-options obs-store)

    if (HDF5_VERSION VERSION_GREATER_EQUAL 1.12.0)
        add_test(NAME test_ioda-engines_dim-selectors-default COMMAND ioda-engines_dim-selectors)
        add_test(NAME test_ioda-engines_dim-selectors-h5file COMMAND ioda-engines_dim-selectors --ioda-engine-options HDF5-file "dim-selectors-file.hdf5" create truncate)
        add_test(NAME test_ioda-engines_dim-selectors-h5mem COMMAND ioda-engines_dim-selectors --ioda-engine-options HDF5-mem "dim-selectors-mem.hdf5" 10 false)
    endif()
    add_test(NAME test_ioda-engines_dim-selectors-ObsStore COMMAND ioda-engines_dim-selectors --ioda-engine-options obs-store)
endif()
