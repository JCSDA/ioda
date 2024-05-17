# (C) Copyright 2021-2024 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Demonstrate the ObsDataFrame (odf) capabilities
ecbuild_add_executable( TARGET  ioda-odfDemo.x
                        SOURCES odfDemo.cpp
                        LIBS    ioda )


