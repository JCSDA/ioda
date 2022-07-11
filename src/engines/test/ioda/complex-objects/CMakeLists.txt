# (C) Copyright 2020-2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_complex_objects_strings strings.cpp)
addapp(ioda-engines_complex_objects_strings)
target_link_libraries(ioda-engines_complex_objects_strings PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_complex_objects_strings-default COMMAND ioda-engines_complex_objects_strings)
    add_test(NAME test_ioda-engines_complex_objects_strings-h5file COMMAND ioda-engines_complex_objects_strings --ioda-engine-options HDF5-file "engines_complex_objects_strings-file.hdf5" create truncate)
    add_test(NAME test_ioda-engines_complex_objects_strings-h5mem COMMAND ioda-engines_complex_objects_strings --ioda-engine-options HDF5-mem "engines_complex_objects_strings-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines_complex_objects_strings-ObsStore COMMAND ioda-engines_complex_objects_strings --ioda-engine-options obs-store)

endif()
