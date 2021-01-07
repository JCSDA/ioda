# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

macro(AddExample ExName SourceFile Group)
	add_executable(${ExName} ${SourceFile})
	ApplyBaseSettings(${ExName})
	# ecbuild does something odd to the RPATHs, breaking this.
	#    if (NOT WIN32)
	#	    set_target_properties( ${ExName}
	#          PROPERTIES
	#      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Examples"
	#    )
	#    endif()
	set_target_properties( ${ExName} PROPERTIES FOLDER "Examples/${Group}")
	target_link_libraries(${ExName} PUBLIC ioda_engines)

	# MSVC has a really bad and old C compiler. Instead use the C++ compiler.
	# For other compilers, force C11, which is the CMake max version.
	if (MSVC)
		set_source_files_properties(${SourceFile} PROPERTIES LANGUAGE CXX)
	else()
		set_property(TARGET ${ExName} PROPERTY C_STANDARD 11)
	endif()
endmacro()

AddExample(ioda-engines-01a-C 01a-GroupsAndObsSpaces.c "ioda/C")
AddExample(ioda-engines-01b-C 01b-GroupsAndObsSpaces.c "ioda/C")
AddExample(ioda-engines-02-C 02-Attributes.c "ioda/C")
AddExample(ioda-engines-03-C 03-VariablesIntro.c "ioda/C")
AddExample(ioda-engines-04-C 04-VariablesAttributesAndDimensions.c "ioda/C")
# Future examples:
# 5. How to create an obs space (for ioda-converters).
# 6. Obs space organization and the ioda data layout.
# 7. Selecting / searching for data using ioda-engines. Creating an Obs Vector.

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines-01a-C-default COMMAND ioda-engines-01a-C)
	add_test(NAME test_ioda-engines-01b-C-default COMMAND ioda-engines-01b-C)

	add_test(NAME test_ioda-engines-01a-C-h5file COMMAND ioda-engines-01a-C --ioda-engine-options HDF5-file "ioda-engines-01a-C.hdf5" create truncate)
	add_test(NAME test_ioda-engines-01a-C-h5mem COMMAND ioda-engines-01a-C --ioda-engine-options HDF5-mem "ioda-engines-01a-C-mem.hdf5" 10 false)
	add_test(NAME test_ioda-engines-01a-C-ObsStore COMMAND ioda-engines-01a-C --ioda-engine-options obs-store)

	add_test(NAME test_ioda-engines-01b-C-h5file COMMAND ioda-engines-01b-C --ioda-engine-options HDF5-file "ioda-engines-01b-C.hdf5" create truncate)
	add_test(NAME test_ioda-engines-01b-C-h5mem COMMAND ioda-engines-01b-C --ioda-engine-options HDF5-mem "ioda-engines-01b-C-mem.hdf5" 10 false)
	add_test(NAME test_ioda-engines-01b-C-ObsStore COMMAND ioda-engines-01b-C --ioda-engine-options obs-store)

	
    add_test(NAME test_ioda-engines-02-C-default COMMAND ioda-engines-02-C)
	add_test(NAME test_ioda-engines-02-C-h5file COMMAND ioda-engines-02-C --ioda-engine-options HDF5-file "ioda-engines-02-C.hdf5" create truncate)
	add_test(NAME test_ioda-engines-02-C-h5mem COMMAND ioda-engines-02-C --ioda-engine-options HDF5-mem "ioda-engines-02-C-mem.hdf5" 10 false)
	add_test(NAME test_ioda-engines-02-C-ObsStore COMMAND ioda-engines-02-C --ioda-engine-options obs-store)

	add_test(NAME test_ioda-engines-03-C-default COMMAND ioda-engines-03-C)
	add_test(NAME test_ioda-engines-03-C-h5file COMMAND ioda-engines-03-C --ioda-engine-options HDF5-file "ioda-engines-03-C.hdf5" create truncate)
	add_test(NAME test_ioda-engines-03-C-h5mem COMMAND ioda-engines-03-C --ioda-engine-options HDF5-mem "ioda-engines-03-C-mem.hdf5" 10 false)
	add_test(NAME test_ioda-engines-03-C-ObsStore COMMAND ioda-engines-03-C --ioda-engine-options obs-store)

	add_test(NAME test_ioda-engines-04-C-default COMMAND ioda-engines-04-C)
	add_test(NAME test_ioda-engines-04-C-h5file COMMAND ioda-engines-04-C --ioda-engine-options HDF5-file "ioda-engines-04-C.hdf5" create truncate)
	add_test(NAME test_ioda-engines-04-C-h5mem COMMAND ioda-engines-04-C --ioda-engine-options HDF5-mem "ioda-engines-04-C-mem.hdf5" 10 false)
	add_test(NAME test_ioda-engines-04-C-ObsStore COMMAND ioda-engines-04-C --ioda-engine-options obs-store)
endif()

