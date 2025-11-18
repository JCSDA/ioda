# (C) Crown Copyright 2021 Met Office
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(test_ioda-engines_hasvariables_convertvariableunits test_convertvariableunits.cpp)
addapp(test_ioda-engines_hasvariables_convertvariableunits)
target_link_libraries(test_ioda-engines_hasvariables_convertvariableunits PUBLIC ioda_engines)
add_test(NAME test_ioda-engines_hasvariables_convertvariableunits COMMAND test_ioda-engines_hasvariables_convertvariableunits)
