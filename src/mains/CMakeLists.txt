# (C) Copyright 2017-2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

ecbuild_add_executable( TARGET  time_IodaIO.x
                        SOURCES timeIodaIO.cc
                        LIBS    ioda )

add_subdirectory( validator )