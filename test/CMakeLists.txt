# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.json
  testinput/iodatest_odb.json
  testinput/iodatest_odb1.json
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)
foreach(FILENAME ${ioda_test_input})
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} )
endforeach(FILENAME)

# Add files to cmake resources
ecbuild_add_resources( TARGET ioda_test_scripts
                       SOURCES_PACK
                       ${ioda_test_input}
                     )

# Create Data directory for test data and symlink files
list( APPEND ioda_test_data
  diag_viirs_ges.2015081000_dbl_subset.nc4  
  amsua_n19_obs.nc4
  diag_t_01_wprofiles.nc4
  diag_t_01_wprofiles.odb
  diag_t_01_wprofiles_odb
  seaice_obs-2018-04-15.nc
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
foreach(FILENAME ${ioda_test_data})
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/testinput/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/Data/${FILENAME} )
endforeach(FILENAME)

#####################################################################

ecbuild_add_test( TARGET  test_ioda_locations
                  BOOST
                  SOURCES mains/TestLocations.cc
                  ARGS    "testinput/iodatest.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_observationspace
                  BOOST
                  SOURCES mains/TestObservationSpace.cc
                  ARGS "testinput/iodatest.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsvector
                  BOOST
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest.json"
                  LIBS    ioda )

if (HAVE_ODB_API)

ecbuild_add_test( TARGET  test_ioda_locations_odb
                  BOOST
                  SOURCES mains/TestLocations.cc
                  ARGS    "testinput/iodatest_odb.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_observationspace_odb
                  BOOST
                  SOURCES mains/TestObservationSpace.cc
                  ARGS "testinput/iodatest_odb.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsvector_odb
                  BOOST
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest_odb.json"
                  LIBS    ioda )

endif (HAVE_ODB_API)

if (HAVE_ODB)

ecbuild_add_test( TARGET  test_ioda_locations_odb1
                  BOOST
                  SOURCES mains/TestLocations.cc
                  ARGS    "testinput/iodatest_odb1.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_observationspace_odb1
                  BOOST
                  SOURCES mains/TestObservationSpace.cc
                  ARGS "testinput/iodatest_odb1.json"
                  LIBS    ioda OOPS )

ecbuild_add_test( TARGET test_ioda_obsvector_odb1
                  BOOST
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest_odb1.json"
                  LIBS    ioda OOPS )

endif (HAVE_ODB)

