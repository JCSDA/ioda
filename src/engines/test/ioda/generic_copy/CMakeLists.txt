# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

add_executable(ioda_generic-copy copy1.cpp)
addapp(ioda_generic-copy)
target_link_libraries(ioda_generic-copy PUBLIC ioda_engines)

if(0) #BUILD_TESTING)
    add_test(NAME ioda_generic-copy-default COMMAND ioda_generic-copy)
    add_test(NAME ioda_generic-copy-h5file COMMAND ioda_generic-copy --ioda-engine-options HDF5-file "fills.hdf5" create truncate)
    add_test(NAME ioda_generic-copy-h5mem COMMAND ioda_generic-copy --ioda-engine-options HDF5-mem "fills-mem.hdf5" 10 false)
    add_test(NAME ioda_generic-copy-ObsStore COMMAND ioda_generic-copy --ioda-engine-options obs-store)
endif()

