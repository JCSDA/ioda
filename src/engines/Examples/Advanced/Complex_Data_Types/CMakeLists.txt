# (C) Copyright 2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

macro(AddExample ExName SourceFile Group)
	add_executable(${ExName} ${SourceFile})
	ApplyBaseSettings(${ExName})
	set_target_properties( ${ExName} PROPERTIES FOLDER "Examples/${Group}")
	target_link_libraries(${ExName} PUBLIC ioda_engines)

    add_test(NAME test_${ExName}-default COMMAND ${ExName})
    add_test(NAME test_${ExName}-h5file COMMAND ${ExName} --ioda-engine-options HDF5-file "${ExName}-file.hdf5" create truncate)
    add_test(NAME test_${ExName}-h5mem COMMAND ${ExName} --ioda-engine-options HDF5-mem "${ExName}-mem.hdf5" 10 false)

    # ObsStore disabled for now. Features will be added in subsequent PRs.
    #add_test(NAME test_${ExName}-ObsStore COMMAND ${ExName} --ioda-engine-options obs-store)
endmacro()

AddExample(ioda-engines_complex_objects_array_from_struct array_from_struct.cpp "ioda/Advanced/Complex Data Types")
AddExample(ioda-engines_fixed_length_strings fixed_length_strings.cpp "ioda/Advanced/Complex Data Types")
AddExample(ioda-engines_fixed_length_strings_client fixed_length_strings_client.cpp "ioda/Advanced/Complex Data Types")
AddExample(ioda-engines_named_types named_types.cpp "ioda/Advanced/Complex Data Types")
