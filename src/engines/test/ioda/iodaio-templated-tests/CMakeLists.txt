# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_io_templated_tests test.cpp)
addapp(ioda-engines_io_templated_tests)
target_link_libraries(ioda-engines_io_templated_tests PUBLIC ioda_engines)

add_executable(ioda-engines_hier_paths test_hier_paths.cpp)
addapp(ioda-engines_hier_paths)
target_link_libraries(ioda-engines_hier_paths PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_io_templated_tests-default COMMAND ioda-engines_io_templated_tests)
    add_test(NAME test_ioda-engines_io_templated_tests-h5file COMMAND ioda-engines_io_templated_tests --ioda-engine-options HDF5-file "IODAio_templated_tests-file.hdf5" create truncate)
    add_test(NAME test_ioda-engines_io_templated_tests-h5mem COMMAND ioda-engines_io_templated_tests --ioda-engine-options HDF5-mem "IODAio_templated_tests-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines_io_templated_tests-ObsStore COMMAND ioda-engines_io_templated_tests --ioda-engine-options obs-store)

    add_test(NAME test_ioda-engines_hier_paths-default COMMAND ioda-engines_hier_paths)
    add_test(NAME test_ioda-engines_hier_paths-h5file COMMAND ioda-engines_hier_paths --ioda-engine-options HDF5-file "IODAhier_paths-file.hdf5" create truncate)
    add_test(NAME test_ioda-engines_hier_paths-h5mem COMMAND ioda-engines_hier_paths --ioda-engine-options HDF5-mem "IODAhier_paths-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines_hier_paths-ObsStore COMMAND ioda-engines_hier_paths --ioda-engine-options obs-store)
endif()
