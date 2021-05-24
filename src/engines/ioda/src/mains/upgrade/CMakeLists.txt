# (C) Copyright 2020-2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Program to upgrade a ioda file's format to a newer version of
# the ioda standard.

include(Targets)

add_executable(ioda-upgrade.x upgrade.cpp)
addapp(ioda-upgrade.x)
target_link_libraries(ioda-upgrade.x PUBLIC ioda_engines)

install(TARGETS ioda-upgrade.x RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# No tests are defined here since ioda-upgrade.x is tested in the ioda top-level ctests.
# See test/CMakeLists.txt configuration for details.
