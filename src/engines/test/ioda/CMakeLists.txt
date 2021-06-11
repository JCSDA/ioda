# (C) Copyright 2020-2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

add_subdirectory(collective_functions)
add_subdirectory(complex-objects)
add_subdirectory(chunks_and_filters)
add_subdirectory(data-selections)
add_subdirectory(engine-hdf5)
add_subdirectory(exceptions)
add_subdirectory(fillvalues)
# Needs development: add_subdirectory(generic_copy)
add_subdirectory(iodaio-templated-tests)
add_subdirectory(layouts)
add_subdirectory(obsgroup)
add_subdirectory(misc)
add_subdirectory(persist)
add_subdirectory(list-objects)
add_subdirectory(variables)
# Needs a test file
#add_subdirectory(upgrade_format)

add_subdirectory(C)

if(BUILD_PYTHON_BINDINGS)
	add_subdirectory(python)
endif(BUILD_PYTHON_BINDINGS)

