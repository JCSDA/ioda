# (C) Crown copyright 2021, Met Office
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda-engines_odb-parallel-io test_odb_parallel_io.cpp)
addapp(ioda-engines_odb-parallel-io)
target_link_libraries(ioda-engines_odb-parallel-io PUBLIC ioda_engines)
target_include_directories(ioda-engines_odb-parallel-io PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../ioda/include/ioda)

if(BUILD_TESTING)
  ecbuild_add_test( TARGET  ioda-engines_odb-parallel-io
                    MPI     3
                    COMMAND ioda-engines_odb-parallel-io
                    ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/odb_parallel_io.yml)
endif()
