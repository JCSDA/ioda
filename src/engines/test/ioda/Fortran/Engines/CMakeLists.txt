# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

# Tests are split because of HDF5 1.8 on discover.

add_executable(ioda_f_engines_create test-Engines.f90)
addapp(ioda_f_engines_create)
target_link_libraries(ioda_f_engines_create PUBLIC ioda_engines ioda_fortran)

# MSVC has a really bad and old C compiler. Instead use the C++ compiler.
set_source_files_properties(test-Engines.f90 PROPERTIES LANGUAGE Fortran)

add_executable(ioda_f_engines_open test-Engines-open.f90)
addapp(ioda_f_engines_open)
target_link_libraries(ioda_f_engines_open PUBLIC ioda_engines ioda_fortran)

# MSVC has a really bad and old C compiler. Instead use the C++ compiler.
set_source_files_properties(test-Engines-open.f90 PROPERTIES LANGUAGE Fortran)


if(BUILD_TESTING)
    add_test(NAME ioda_f_engines_create COMMAND ioda_f_engines_create)
    add_test(NAME ioda_f_engines_open COMMAND ioda_f_engines_open)
    set_tests_properties(ioda_f_engines_open PROPERTIES DEPENDS ioda_f_engines_create)
endif()
