# (C) Crown Copyright 2021 Met Office
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

# Although these tests are for features used by the odb interface, they do not use
# odb / odc itself.

add_executable(test_ioda-engines_layouts_layoutobsgroupodb test-layoutodb.cpp)
addapp(test_ioda-engines_layouts_layoutobsgroupodb)
target_link_libraries(test_ioda-engines_layouts_layoutobsgroupodb PUBLIC ioda_engines)
add_test(NAME test_ioda-engines_layouts_layoutobsgroupodb COMMAND test_ioda-engines_layouts_layoutobsgroupodb)
target_include_directories(test_ioda-engines_layouts_layoutobsgroupodb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../ioda/src/ioda)

add_executable(test_ioda-engines_layouts_layoutobsgroup test-layoutobsgroup.cpp)
addapp(test_ioda-engines_layouts_layoutobsgroup)
target_link_libraries(test_ioda-engines_layouts_layoutobsgroup PUBLIC ioda_engines)
add_test(NAME test_ioda-engines_layouts_layoutobsgroup COMMAND test_ioda-engines_layouts_layoutobsgroup)
target_include_directories(test_ioda-engines_layouts_layoutobsgroup PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../ioda/src/ioda)

# Include ioda-internal headers for these tests.
target_include_directories(test_ioda-engines_layouts_layoutobsgroupodb PUBLIC
  $<BUILD_INTERFACE:${IODA_ENGINES_PRIVATE_DIR}>)

target_include_directories(test_ioda-engines_layouts_layoutobsgroup PUBLIC
  $<BUILD_INTERFACE:${IODA_ENGINES_PRIVATE_DIR}>)
