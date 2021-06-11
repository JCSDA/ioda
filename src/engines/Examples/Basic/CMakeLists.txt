# (C) Copyright 2020-2021 UCAR.
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

endmacro()

AddExample(ioda-engines-01 01-GroupsAndObsSpaces.cpp "ioda/Basic")
AddExample(ioda-engines-02 02-Attributes.cpp "ioda/Basic")
AddExample(ioda-engines-03 03-VariablesIntro.cpp "ioda/Basic")
AddExample(ioda-engines-04 04-VariablesAttributesAndDimensions.cpp "ioda/Basic")
AddExample(ioda-engines-05a 05a-ObsGroup.cpp "ioda/Basic")
AddExample(ioda-engines-05b 05b-ObsGroupAppend.cpp "ioda/Basic")

# Future examples:
# 6. Obs space organization and the ioda data layout.
# 7. Selecting / searching for data using ioda-engines. Creating an Obs Vector.

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines-01-default COMMAND ioda-engines-01)
    add_test(NAME test_ioda-engines-02-default COMMAND ioda-engines-02)
    add_test(NAME test_ioda-engines-03-default COMMAND ioda-engines-03)
    add_test(NAME test_ioda-engines-04-default COMMAND ioda-engines-04)
    add_test(NAME test_ioda-engines-05a-default COMMAND ioda-engines-05a)
    add_test(NAME test_ioda-engines-05b-default COMMAND ioda-engines-05b)

    add_test(NAME test_ioda-engines-01-h5file COMMAND ioda-engines-01 --ioda-engine-options HDF5-file "ioda-engines-01.hdf5" create truncate)
    add_test(NAME test_ioda-engines-02-h5file COMMAND ioda-engines-02 --ioda-engine-options HDF5-file "ioda-engines-02.hdf5" create truncate)
    add_test(NAME test_ioda-engines-03-h5file COMMAND ioda-engines-03 --ioda-engine-options HDF5-file "ioda-engines-03.hdf5" create truncate)
    add_test(NAME test_ioda-engines-04-h5file COMMAND ioda-engines-04 --ioda-engine-options HDF5-file "ioda-engines-04.hdf5" create truncate)
    add_test(NAME test_ioda-engines-05a-h5file COMMAND ioda-engines-05a --ioda-engine-options HDF5-file "ioda-engines-05a.hdf5" create truncate)
    add_test(NAME test_ioda-engines-05b-h5file COMMAND ioda-engines-05b --ioda-engine-options HDF5-file "ioda-engines-05b.hdf5" create truncate)

    add_test(NAME test_ioda-engines-01-h5mem COMMAND ioda-engines-01 --ioda-engine-options HDF5-mem "ioda-engines-01-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines-02-h5mem COMMAND ioda-engines-02 --ioda-engine-options HDF5-mem "ioda-engines-02-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines-03-h5mem COMMAND ioda-engines-03 --ioda-engine-options HDF5-mem "ioda-engines-03-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines-04-h5mem COMMAND ioda-engines-04 --ioda-engine-options HDF5-mem "ioda-engines-04-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines-05a-h5mem COMMAND ioda-engines-05a --ioda-engine-options HDF5-mem "ioda-engines-05a-mem.hdf5" 10 false)
    add_test(NAME test_ioda-engines-05b-h5mem COMMAND ioda-engines-05b --ioda-engine-options HDF5-mem "ioda-engines-05b-mem.hdf5" 10 false)

    add_test(NAME test_ioda-engines-01-ObsStore COMMAND ioda-engines-01 --ioda-engine-options obs-store)
    add_test(NAME test_ioda-engines-02-ObsStore COMMAND ioda-engines-02 --ioda-engine-options obs-store)
    add_test(NAME test_ioda-engines-03-ObsStore COMMAND ioda-engines-03 --ioda-engine-options obs-store)
    add_test(NAME test_ioda-engines-04-ObsStore COMMAND ioda-engines-04 --ioda-engine-options obs-store)
    add_test(NAME test_ioda-engines-05a-ObsStore COMMAND ioda-engines-05a --ioda-engine-options obs-store)
    add_test(NAME test_ioda-engines-05b-ObsStore COMMAND ioda-engines-05b --ioda-engine-options obs-store)
endif()

