include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

list( APPEND ioda_src_files

ioda/ObsData.cc
ioda/ObsData.h
ioda/ObsDataVector.h
ioda/ObsDataVector.cc
ioda/ObsSpace.cc
ioda/ObsSpace.h
ioda/ObsVector.cc
ioda/ObsVector.h
ioda/obsspace_f.cc
ioda/obsspace_f.h
ioda/obsspace_mod.F90

database/ObsSpaceContainer.cc
database/ObsSpaceContainer.h

distribution/Distribution.cc
distribution/Distribution.h
distribution/DistributionFactory.cc
distribution/DistributionFactory.h
distribution/InefficientDistribution.cc
distribution/InefficientDistribution.h
distribution/RoundRobin.cc
distribution/RoundRobin.h

fileio/IodaIO.cc
fileio/IodaIO.h
fileio/IodaIOfactory.cc
fileio/IodaIOfactory.h
fileio/NetcdfIO.cc
fileio/NetcdfIO.h

utils/IodaUtils.cc
utils/IodaUtils.h
)

ecbuild_add_library( TARGET   ioda
                     SOURCES  ${ioda_src_files}
                     LIBS     oops ${IODA_EXTRA_LIBRARIES}
                     INSTALL_HEADERS LISTED
                     LINKER_LANGUAGE ${IODA_LINKER_LANGUAGE}
                    )

# coding norms check
ecbuild_add_test( TARGET ioda_coding_norms
                  TYPE SCRIPT
                  COMMAND cpplint.py
                  ARGS --quiet --recursive ${CMAKE_CURRENT_SOURCE_DIR}
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

