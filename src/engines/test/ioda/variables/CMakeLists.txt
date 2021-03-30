# (C) Crown Copyright 2021 Met Office
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

ecbuild_add_test ( TARGET     test_ioda-engines_hasvariables_stitchcomplementaryvars
                   SOURCES    test.cpp
                   LIBS       ioda_engines )

target_compile_definitions(test_ioda-engines_hasvariables_stitchcomplementaryvars PRIVATE
  TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
