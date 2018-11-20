# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.yml
  testinput/iodatest_odb.json
  testinput/iodatest_odb1.json
  testinput/iodatest_obsspace.json
  testinput/iodatest_obsvector.yml
  testinput/iodatest_obsvector_odb.yml
  testinput/iodatest_fileio.yml
  testinput/iodatest_marine_fileio.yml
  testinput/iodatest_marine_obsspace.yml  
  testinput/iodatest_fileio_odb_api.yml
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testinput)
foreach(FILENAME ${ioda_test_input})
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} )
endforeach(FILENAME)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testoutput)

# Add files to cmake resources
ecbuild_add_resources( TARGET ioda_test_scripts
                       SOURCES_PACK
                       ${ioda_test_input}
                     )

# Create Data directory for test data and symlink files
list( APPEND ioda_test_data
  atmosphere/aircraft_obs_2018041500_m.nc4
  atmosphere/aircraft_obs_2018041500_s.nc4
  atmosphere/amsua_obs_n19_2018041500_m.nc4
  atmosphere/amsua_obs_n19_2018041500_s.nc4
  atmosphere/sondes_obs_2018041500_m.nc4
  atmosphere/sondes_obs_2018041500_s.nc4
  atmosphere/diag_t_obs_01_wprofiles.odb
  atmosphere/diag_t_obs_01_wprofiles_odb

  constituents/diag_viirs_ges.2015081000_dbl_subset.nc4
  constituents/aod_obs_2015081000_m.nc4

  marine/Jason-2-2018-04-15.nc
  marine/profile_2018-04-15.nc
  marine/cryosat2-2018-04-15.nc
  marine/icec-2018-04-15.nc
  marine/sst_obs-2018-04-15.nc4
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
foreach(FILENAME ${ioda_test_data})
    get_filename_component(filename ${FILENAME} NAME )
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/testinput/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/Data/${filename} )
endforeach(FILENAME)

#####################################################################

ecbuild_add_test( TARGET  test_ioda_locations
                  BOOST
                  SOURCES mains/TestLocations.cc
                  ARGS    "testinput/iodatest.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_observationspace
                  BOOST
                  SOURCES mains/TestObservationSpace.cc
                  ARGS "testinput/iodatest.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_oops_obsvector
                  BOOST
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest_obsvector.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsvector
                  BOOST
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS "testinput/iodatest_obsvector.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsspace
                  BOOST
                  SOURCES mains/TestObsSpace.cc
                  ARGS "testinput/iodatest_obsspace.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_marine_obsspace
                  BOOST
                  SOURCES mains/TestObsSpace.cc
                  ARGS "testinput/iodatest_marine_obsspace.yml"
                  LIBS    ioda )

		
ecbuild_add_test( TARGET test_ioda_fileio
                  BOOST
                  SOURCES mains/TestIodaIO.cc
                  ARGS "testinput/iodatest_fileio.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_marine_fileio
                  BOOST
                  SOURCES mains/TestIodaIO.cc
                  ARGS "testinput/iodatest_marine_fileio.yml"
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

ecbuild_add_test( TARGET test_ioda_oops_obsvector_odb
                  BOOST
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest_odb.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsvector_odb
                  BOOST
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS "testinput/iodatest_obsvector_odb.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_fileio_odb_api
                  BOOST
                  SOURCES mains/TestIodaIO.cc
                  ARGS "testinput/iodatest_fileio_odb_api.yml"
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

