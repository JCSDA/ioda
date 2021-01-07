# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

add_subdirectory(Basic)
add_subdirectory(C)

if(BUILD_PYTHON_BINDINGS)
	add_subdirectory(Python)
endif(BUILD_PYTHON_BINDINGS)

