# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

add_subdirectory(complex-objects)
add_subdirectory(chunks_and_filters)
add_subdirectory(data-selections)
add_subdirectory(engine-hdf5)
add_subdirectory(fillvalues)
# Needs development: add_subdirectory(generic_copy)
add_subdirectory(iodaio-templated-tests)
add_subdirectory(obsgroup)
add_subdirectory(misc)
add_subdirectory(persist)
# Needs a test file
#add_subdirectory(upgrade_format)

add_subdirectory(C)

if(BUILD_PYTHON_BINDINGS)
	add_subdirectory(python)
endif(BUILD_PYTHON_BINDINGS)

