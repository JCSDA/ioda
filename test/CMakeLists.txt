# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.json
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
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
foreach(FILENAME ${ioda_test_data})
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/testinput/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/Data/${FILENAME} )
endforeach(FILENAME)

# Link to UFO Marine obs
list( APPEND marine_obs
  ../../ufo/test/testinput/marineobs/seaice_obs-2018-04-15.nc
)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
foreach(FILENAME ${marine_obs})
     get_filename_component(filename ${FILENAME} NAME )
     execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}    
           ${CMAKE_CURRENT_BINARY_DIR}/Data/${filename} )
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

