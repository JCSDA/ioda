# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

macro(AddPyExample_base name testname)
	add_test(NAME ${testname}
		COMMAND #${Python3_EXECUTABLE}
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py )
	set_tests_properties(${testname} PROPERTIES
		ENVIRONMENT "LIBDIR=$<TARGET_FILE_DIR:_ioda_python>/../;LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}"
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
		)
endmacro()

macro(AddPyExample name )
	set(testname Example_ioda_engines_${name}_python)
    AddPyExample_base(${name} ${testname})
endmacro()

if(BUILD_TESTING)
    #AddPyExample_base(check_cartopy ioda_check_cartopy)
    #set_tests_properties(ioda_check_cartopy PROPERTIES
    #    SKIP_RETURN_CODE 1
    #    FIXTURES_SETUP PLOTS)

	AddPyExample(01-GroupsAndObsSpaces)
	AddPyExample(02-Attributes)
	AddPyExample(03-VariablesIntro)
	AddPyExample(04-VariablesAttributesAndDimensions)
	AddPyExample(05-ObsGroup)
    #AddPyExample(plotting-05-ObsGroup)
    #set_tests_properties(Example_ioda_engines_plotting-05-ObsGroup_python
    #    PROPERTIES
    #    DEPENDS Example_ioda_engines_05-ObsGroup_python
    #    FIXTURES_REQUIRED PLOTS
    #    ATTACHED_FILES ${PROJECT_BINARY_DIR}/plotting-Example-05a-python.png
    #    SKIP_RETURN_CODE 1)
endif()

