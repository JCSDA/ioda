# (C) Copyright 2020-2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_exception test_exceptions.cpp)
addapp(ioda-engines_exception)
target_link_libraries(ioda-engines_exception PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_exception COMMAND ioda-engines_exception)
endif()
