# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

if(BUILD_TESTING)
	add_test(NAME ioda-python COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/funcs.py)
	set_tests_properties(ioda-python PROPERTIES
		ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:_ioda_python>/../../:$ENV{PYTHONPATH};LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}"
		)
endif()

