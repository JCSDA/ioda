# (C) Copyright 2020-2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

macro(AddPyExample_base name testname args)
	add_test(NAME ${testname}
		COMMAND ${Python3_EXECUTABLE}
			${CMAKE_CURRENT_SOURCE_DIR}/${name}.py ${args}
		)
	set(IODA_PYTHONPATH "$<TARGET_FILE_DIR:_ioda_python>/../:$ENV{PYTHONPATH}")
	set_tests_properties(${testname}
		PROPERTIES
		ENVIRONMENT "PYTHONPATH=${IODA_PYTHONPATH}"
		)
endmacro()

macro(AddPyExample name args)
	set(testname Example_ioda_engines_${name}_python)
    AddPyExample_base(${name} ${testname} "${args}")
endmacro()

if(BUILD_TESTING)
    #AddPyExample_base(check_cartopy ioda_check_cartopy)
    #set_tests_properties(ioda_check_cartopy PROPERTIES
    #    SKIP_RETURN_CODE 1
    #    FIXTURES_SETUP PLOTS)
	set( IODA_TEST_DATA_DIR ${CMAKE_BINARY_DIR}/ioda/test/Data/testinput_tier_1 )

	AddPyExample(01-GroupsAndObsSpaces "")
	AddPyExample(02-Attributes "")
	AddPyExample(03-VariablesIntro "")
	AddPyExample(04-VariablesAttributesAndDimensions "")
	AddPyExample(05-ObsGroup "")
	AddPyExample(06-ReadIodaOutputFile ${IODA_TEST_DATA_DIR}/sample_hofx_output_amsua_n19.nc4 )
	AddPyExample(07-ObsSpaceClass ${IODA_TEST_DATA_DIR}/sample_hofx_output_amsua_n19.nc4 )
    #AddPyExample(plotting-05-ObsGroup)
    #set_tests_properties(Example_ioda_engines_plotting-05-ObsGroup_python
    #    PROPERTIES
    #    DEPENDS Example_ioda_engines_05-ObsGroup_python
    #    FIXTURES_REQUIRED PLOTS
    #    ATTACHED_FILES ${PROJECT_BINARY_DIR}/plotting-Example-05a-python.png
    #    SKIP_RETURN_CODE 1)
endif()
