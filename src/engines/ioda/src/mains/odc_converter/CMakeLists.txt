# (C) Copyright 2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Program to read in an ODB database and write an HDF5 ioda file.
# For degugging purposes.

include(Targets)

add_executable(ioda-odc-converter.x odc_converter.cpp)
addapp(ioda-odc-converter.x)
target_link_libraries(ioda-odc-converter.x PUBLIC ioda_engines)

install(TARGETS ioda-odc-converter.x RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# No tests are defined here since ioda-odc-converter.x is tested
# in the ioda top-level ctests.
# See test/CMakeLists.txt configuration for details.
