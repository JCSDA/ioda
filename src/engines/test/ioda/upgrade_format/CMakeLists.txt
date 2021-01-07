# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda_engines_oldfiles test_oldfiles.cpp)
addapp(ioda_engines_oldfiles)
target_link_libraries(ioda_engines_oldfiles PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME test_ioda_engines_oldfiles COMMAND ioda_engines_oldfiles)
endif()
