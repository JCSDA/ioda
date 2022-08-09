# (C) Copyright 2020-2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

set( IODA_TEST_DATA_DIR ${CMAKE_BINARY_DIR}/ioda/test/Data/testinput_tier_1 )

macro(AddExample_Base ExName SourceFile )
	add_executable(${ExName} ${SourceFile})
	ApplyBaseSettings(${ExName})
	# ecbuild does something odd to the RPATHs, breaking this.
	#    if (NOT WIN32)
	#	    set_target_properties( ${ExName}
	#          PROPERTIES
	#      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Examples"
	#    )
	#    endif()
	set_target_properties( ${ExName} PROPERTIES FOLDER "Examples/ioda/Basic")
	target_link_libraries(${ExName} PUBLIC ioda_engines)
endmacro()

macro(AddExample_CXX ExName SourceFile )
    AddExample_Base( ${ExName} ${SourceFile} )

	add_test(NAME test_${ExName}-default COMMAND ${ExName})
	add_test(NAME test_${ExName}-h5file COMMAND ${ExName} --ioda-engine-options HDF5-file "${ExName}-h5file.hdf5" create truncate)
	add_test(NAME test_${ExName}-h5mem COMMAND ${ExName} --ioda-engine-options HDF5-mem "${ExName}-h5mem.hdf5" 10 false)
	add_test(NAME test_${ExName}-obsstore COMMAND ${ExName} --ioda-engine-options obs-store)
endmacro()

macro(AddExample_C ExName SourceFile )
	AddExample_Base( ${ExName} ${SourceFile} )
	# MSVC has a really bad and old C compiler. Instead use the C++ compiler.
	# For other compilers, force C11, which is the CMake max version.
	if (MSVC)
		set_source_files_properties(${SourceFile} PROPERTIES LANGUAGE CXX)
	else()
		set_property(TARGET ${ExName} PROPERTY C_STANDARD 11)
	endif()
	add_test(NAME test_${ExName} COMMAND ${ExName})
endmacro()

macro(AddExample_F ExName SourceFile )
	AddExample_Base( ${ExName} ${SourceFile} )
	target_link_libraries(${ExName} PUBLIC ioda_fortran)
	add_test(NAME test_${ExName} COMMAND ${ExName})
endmacro()

macro(AddExample_Py testname cmd args)
	add_test(NAME test_${testname}
		COMMAND ${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/${cmd} ${args}
		)
	set(IODA_PYTHONPATH "$<TARGET_FILE_DIR:_ioda_python>/../:$ENV{PYTHONPATH}")
	set_tests_properties(test_${testname}
		PROPERTIES
		ENVIRONMENT "PYTHONPATH=${IODA_PYTHONPATH}"
		)
endmacro()


AddExample_CXX (ioda-engines-01 01-GroupsAndObsSpaces.cpp)
AddExample_CXX (ioda-engines-02 02-Attributes.cpp)
AddExample_CXX (ioda-engines-03 03-VariablesIntro.cpp)
AddExample_CXX (ioda-engines-04 04-VariablesAttributesAndDimensions.cpp)
AddExample_CXX (ioda-engines-05a 05a-ObsGroup.cpp)
AddExample_CXX (ioda-engines-05b 05b-ObsGroupAppend.cpp)

AddExample_C   (ioda-engines-00-Strings-C 00-Strings.c)
AddExample_C   (ioda-engines-00-VecStrings-C 00-VecStrings.c)
AddExample_C   (ioda-engines-01-C 01-GroupsAndObsSpaces.c)
# Disabled, pending future PRs to update the C interfaces.
#AddExample_C   (ioda-engines-02-C 02-Attributes.c)
#AddExample_C   (ioda-engines-03-C 03-VariablesIntro.c)
#AddExample_C   (ioda-engines-04-C 04-VariablesAttributesAndDimensions.c)

if (IODA_BUILD_LANGUAGE_FORTRAN)
	AddExample_F   (ioda-engines-00-Strings-F 00-Strings.f90)
	AddExample_F   (ioda-engines-00-VecStrings-F 00-VecStrings.f90)
	AddExample_F   (ioda-engines-01-GroupsAndObsSpaces-F 01-GroupsAndObsSpaces.f90)
endif()

if(BUILD_PYTHON_BINDINGS)
	AddExample_Py  (ioda-engines-01-Py 01-GroupsAndObsSpaces.py "")
	AddExample_Py  (ioda-engines-02-Py 02-Attributes.py "")
	AddExample_Py  (ioda-engines-03-Py 03-VariablesIntro.py "")
	AddExample_Py  (ioda-engines-04-Py 04-VariablesAttributesAndDimensions.py "")
	AddExample_Py  (ioda-engines-05-Py 05-ObsGroup.py "")
	AddExample_Py  (ioda-engines-06-Py 06-ReadIodaOutputFile.py ${IODA_TEST_DATA_DIR}/sample_hofx_output_amsua_n19.nc4 )
	AddExample_Py  (ioda-engines-07a-Py-ObsSpaceClass 07a-ObsSpaceClass.py ${IODA_TEST_DATA_DIR}/sample_hofx_output_amsua_n19.nc4 )
	AddExample_Py  (ioda-engines-07b-Py-ObsSpaceClassDataTypes 07b-ObsSpaceClassDataTypes.py "")

	# In the future, this will move to the Advanced examples section.
	AddExample_Py  (ioda-engines-chrono-Py chrono.py "")
endif()

