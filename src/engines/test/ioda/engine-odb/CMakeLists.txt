# (C) Crown copyright 2021, Met Office
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

# Although this test is for features used by the odb interface, it does not use
# odb / odc itself.
if(eckit_FOUND AND oops_FOUND)
  add_executable(ioda-engines_odb-query-parameters test_odb_query_parameters.cpp)
  addapp(ioda-engines_odb-query-parameters)
  target_link_libraries(ioda-engines_odb-query-parameters PUBLIC ioda_engines)
  target_include_directories(ioda-engines_odb-query-parameters PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../ioda/src/ioda)

  if(BUILD_TESTING)
    add_test(NAME test_ioda-engines_odb-query-parameters
             COMMAND ioda-engines_odb-query-parameters ${CMAKE_CURRENT_SOURCE_DIR}/odbqueryparams.yml)
  endif()
endif()
