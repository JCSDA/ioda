# (C) Crown Copyright 2021 Met Office
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

ecbuild_add_test ( TARGET     test_ioda-engines_layouts_layoutobsgroupodb
                   SOURCES    test-layoutodb.cpp
                   LIBS       ioda_engines )

ecbuild_add_test ( TARGET     test_ioda-engines_layouts_layoutobsgroup
                   SOURCES    test-layoutobsgroup.cpp
                   LIBS       ioda_engines )

target_compile_definitions(test_ioda-engines_layouts_layoutobsgroupodb PRIVATE
  TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(test_ioda-engines_layouts_layoutobsgroupodb PUBLIC
  $<BUILD_INTERFACE:${IODA_ENGINES_PRIVATE_DIR}>)

target_include_directories(test_ioda-engines_layouts_layoutobsgroup PUBLIC
  $<BUILD_INTERFACE:${IODA_ENGINES_PRIVATE_DIR}>)
