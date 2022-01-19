# (C) Copyright 2021-2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Validate a ioda file

ecbuild_add_executable( TARGET  ioda-validate.x
                        SOURCES validate.cpp
                          AttributeChecks.cpp
                          AttributeChecks.h
                          Log.cpp
                          Log.h
                          Params.cpp
                          Params.h
                          Units.cpp
                          Units.h
                        LIBS    ioda udunits::udunits )


