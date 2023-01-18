# (C) Copyright 2020-2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Program to upgrade a ioda file's format to a newer version of
# the ioda standard.

ecbuild_add_executable( TARGET ioda-upgrade-v1-to-v2.x
                        SOURCES upgradev1v2.cpp
                        LIBS    ioda )

ecbuild_add_executable( TARGET ioda-upgrade-v2-to-v3.x
                        SOURCES upgradev2v3.cpp
                         ../../../../../mains/validator/AttributeChecks.cpp
                         ../../../../../mains/validator/AttributeChecks.h
                         ../../../../../mains/validator/Log.cpp
                         ../../../../../mains/validator/Log.h
                         ../../../../../mains/validator/Params.cpp
                         ../../../../../mains/validator/Params.h
                        LIBS    ioda )

# No tests are defined here since ioda-upgrade.x is tested in the ioda top-level ctests.
# See test/CMakeLists.txt configuration for details.
