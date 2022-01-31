# (C) Copyright 2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

macro(AddExample ExName SourceFile Group)
	add_executable(${ExName} ${SourceFile})
	ApplyBaseSettings(${ExName})
	set_target_properties( ${ExName} PROPERTIES FOLDER "Examples/${Group}")
	target_link_libraries(${ExName} PUBLIC ioda_engines)

    add_test(NAME test_${ExName} COMMAND ${ExName})
endmacro()

AddExample(ioda-engines_units units.cpp "ioda/Advanced/Math")
AddExample(ioda-engines_basic_math basic_math.cpp "ioda/Advanced/Math")
AddExample(ioda-engines_variables_math variables_math.cpp "ioda/Advanced/Math")
