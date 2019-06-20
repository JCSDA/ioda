# (C) Copyright 2017 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

# Create Data directory for test input config and symlink all files
list( APPEND ioda_test_input
  testinput/iodatest.yml
  testinput/iodatest_odb.json
  testinput/iodatest_odb1.json
  testinput/iodatest_obsspace.yml
  testinput/iodatest_obsvector.yml
  testinput/iodatest_obsvector_odb.yml
  testinput/iodatest_fileio.yml
  testinput/iodatest_distribution.yml
  testinput/iodatest_obsspace_container.yml
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
  atmosphere/amsua_n19_obs_2018041500_m.nc4
  atmosphere/amsua_n19_obs_2018041500_s.nc4
  atmosphere/ioda_metop_2_amsua.nc4
  atmosphere/mhs_n19_obs_2018041500_m.nc4
  atmosphere/atms_npp_obs_2018041500_m.nc4
  atmosphere/cris-fsr_npp_obs_2018041500_m.nc4
  atmosphere/seviri_m08_obs_2018041500_m.nc4
  atmosphere/airs_aqua_obs_2018041500_m.nc4
  atmosphere/airs_aqua_obs_2018041500_s.nc4
  atmosphere/sondes_obs_2018041500_m.nc4
  atmosphere/sondes_obs_2018041500_s.nc4
  atmosphere/diag_t_obs_01_wprofiles.odb
  atmosphere/diag_t_obs_01_wprofiles_odb
  atmosphere/smap_obs_2018041500_m.nc4
  atmosphere/gmi_gpm_obs_2018041500_m.nc4

  atmosphere/aod_obs_2018041500_m.nc4
  atmosphere/hirs4_metop-a_obs_2018041500_m.nc4  
  atmosphere/iasi_metop-a_obs_2018041500_m.nc4
  atmosphere/sndrd1_g15_obs_2018041500_m.nc4
  atmosphere/sndrd2_g15_obs_2018041500_m.nc4
  atmosphere/sndrd3_g15_obs_2018041500_m.nc4
  atmosphere/sndrd4_g15_obs_2018041500_m.nc4

  atmosphere/sfc_obs_2018041500_m.nc4
  atmosphere/sfc_uv_obs_2018041500_m.nc4
  atmosphere/sfc_q_obs_2018041500_m.nc4
  atmosphere/sfc_tsen_obs_2018041500_m.nc4

  marine/Jason-2-2018-04-15.nc
  marine/profile_2018-04-15.nc
  marine/cryosat2-2018-04-15.nc
  marine/icec-2018-04-15.nc
  marine/sst_obs-2018-04-15.nc4

  testfiles/fileio.nc4
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
foreach(FILENAME ${ioda_test_data})
    get_filename_component(filename ${FILENAME} NAME )
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/testinput/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/Data/${filename} )
endforeach(FILENAME)

#####################################################################

ecbuild_add_test( TARGET test_ioda_observationspace
                  SOURCES mains/TestObservationSpace.cc
                  ARGS "testinput/iodatest.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_oops_obsvector
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest_obsvector.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsvector
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS "testinput/iodatest_obsvector.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_distribution
                  MPI     4
                  SOURCES mains/TestDistribution.cc
                  ARGS "testinput/iodatest_distribution.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsspacecontainer
                  SOURCES mains/TestObsSpaceContainer.cc
                  ARGS "testinput/iodatest_obsspace_container.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsspace
                  SOURCES mains/TestObsSpace.cc
                  ARGS "testinput/iodatest_obsspace.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_marine_obsspace
                  SOURCES mains/TestObsSpace.cc
                  ARGS "testinput/iodatest_marine_obsspace.yml"
                  LIBS    ioda )
		
ecbuild_add_test( TARGET test_ioda_fileio
                  SOURCES mains/TestIodaIO.cc
                  ARGS "testinput/iodatest_fileio.yml"
                  LIBS    ioda )

if (HAVE_ODB_API)

ecbuild_add_test( TARGET test_ioda_observationspace_odb
                  SOURCES mains/TestObservationSpace.cc
                  ARGS "testinput/iodatest_odb.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_oops_obsvector_odb
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest_odb.json"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_obsvector_odb
                  SOURCES mains/TestIodaObsVector.cc
                  ARGS "testinput/iodatest_obsvector_odb.yml"
                  LIBS    ioda )

ecbuild_add_test( TARGET test_ioda_fileio_odb_api
                  SOURCES mains/TestIodaIO.cc
                  ARGS "testinput/iodatest_fileio_odb_api.yml"
                  LIBS    ioda )

endif (HAVE_ODB_API)

if (HAVE_ODB)

ecbuild_add_test( TARGET test_ioda_observationspace_odb1
                  SOURCES mains/TestObservationSpace.cc
                  ARGS "testinput/iodatest_odb1.json"
                  LIBS    ioda OOPS )

ecbuild_add_test( TARGET test_ioda_obsvector_odb1
                  SOURCES mains/TestObsVector.cc
                  ARGS "testinput/iodatest_odb1.json"
                  LIBS    ioda OOPS )

endif (HAVE_ODB)

