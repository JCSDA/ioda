# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

# Tests are split because of HDF5 1.8 on discover.

add_executable(ioda_c_engines_create test-Engines.c)
addapp(ioda_c_engines_create)
target_link_libraries(ioda_c_engines_create PUBLIC ioda_engines)

# MSVC has a really bad and old C compiler. Instead use the C++ compiler.
set_source_files_properties(test-Engines.c PROPERTIES LANGUAGE CXX)


add_executable(ioda_c_engines_open test-Engines-open.c)
addapp(ioda_c_engines_open)
target_link_libraries(ioda_c_engines_open PUBLIC ioda_engines)

# MSVC has a really bad and old C compiler. Instead use the C++ compiler.
set_source_files_properties(test-Engines-open.c PROPERTIES LANGUAGE CXX)


if(BUILD_TESTING)
    add_test(NAME ioda_c_engines_create COMMAND ioda_c_engines_create)
    add_test(NAME ioda_c_engines_open COMMAND ioda_c_engines_open)
	set_tests_properties(ioda_c_engines_open PROPERTIES DEPENDS ioda_c_engines_create)
endif()

