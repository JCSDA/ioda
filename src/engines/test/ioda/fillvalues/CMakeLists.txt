# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda_fillvalues ioda-test-fillvalues.cpp)
addapp(ioda_fillvalues)
target_link_libraries(ioda_fillvalues PUBLIC ioda_engines)

if(BUILD_TESTING)
    add_test(NAME ioda_fillvalues-default COMMAND ioda_fillvalues)
    add_test(NAME ioda_fillvalues-h5file COMMAND ioda_fillvalues --ioda-engine-options HDF5-file "fills.hdf5" create truncate)
    add_test(NAME ioda_fillvalues-h5mem COMMAND ioda_fillvalues --ioda-engine-options HDF5-mem "fills-mem.hdf5" 10 false)
    add_test(NAME ioda_fillvalues-ObsStore COMMAND ioda_fillvalues --ioda-engine-options obs-store)
endif()

