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
  testinput/iodatest_obsspace_marine.yml
  testinput/iodatest_fileio_odb_api.yml
  testinput/iodatest_obsspace_fortran.yml
  testinput/iodatest_localobsspace.yml
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
  atmosphere/sondes_obs_2018041500_vs.nc4
  atmosphere/diag_t_obs_01_wprofiles.odb
  atmosphere/diag_t_obs_01_wprofiles_odb
  atmosphere/smap_obs_2018041500_m.nc4
  atmosphere/gmi_gpm_obs_2018041500_m.nc4
  atmosphere/gnssro_obs_2018041500_m.nc4
  atmosphere/gnssro_obs_2018041500_l.nc4

  atmosphere/aod_obs_2018041500_m.nc4
  atmosphere/hirs4_metop-a_obs_2018041500_m.nc4
  atmosphere/iasi_metop-a_obs_2018041500_m.nc4
  atmosphere/sndrd1_g15_obs_2018041500_m.nc4
  atmosphere/sndrd2_g15_obs_2018041500_m.nc4
  atmosphere/sndrd3_g15_obs_2018041500_m.nc4
  atmosphere/sndrd4_g15_obs_2018041500_m.nc4

  atmosphere/ssmis_f18_obs_2018041500_m.nc4

  atmosphere/sfc_obs_2018041500_m.nc4

  marine/Jason-2-2018-04-15.nc
  marine/profile_2018-04-15.nc
  marine/cryosat2-2018-04-15.nc
  marine/icec-2018-04-15.nc
  marine/sst_obs-2018-04-15.nc4

  testfiles/fileio.nc4
  testfiles/sondes_obs_2018041500_m_twfilt.nc4
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Data)
foreach(FILENAME ${ioda_test_data})
    get_filename_component(filename ${FILENAME} NAME )
    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
           ${CMAKE_CURRENT_SOURCE_DIR}/testinput/${FILENAME}
           ${CMAKE_CURRENT_BINARY_DIR}/Data/${filename} )
endforeach(FILENAME)

#####################################################################
# Add executables frequently used in tests

ecbuild_add_executable( TARGET  test_ObsSpace.x
                        SOURCES mains/TestObsSpace.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaObsSpace.x
                        SOURCES mains/TestIodaObsSpace.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaIO.x
                        SOURCES mains/TestIodaIO.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaDistribution.x
                        SOURCES mains/TestDistribution.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_ObsSpaceContainer.x
                        SOURCES mains/TestObsSpaceContainer.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_ObsVector.x
                        SOURCES mains/TestObsVector.cc
                        LIBS    ioda
                       )

ecbuild_add_executable( TARGET  test_IodaObsVector.x
                        SOURCES mains/TestIodaObsVector.cc
                        LIBS    ioda
                       )

#####################################################################

ecbuild_add_test( TARGET  test_ioda_oops_obsvector
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsVector.x
                  ARGS    "testinput/iodatest_obsvector.yml"
                  DEPENDS test_ObsVector.x )

ecbuild_add_test( TARGET  test_ioda_obsvector
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsVector.x
                  ARGS    "testinput/iodatest_obsvector.yml"
                  DEPENDS test_IodaObsVector.x )

ecbuild_add_test( TARGET  test_ioda_distribution
                  MPI     4
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaDistribution.x
                  ARGS    "testinput/iodatest_distribution.yml"
                  DEPENDS test_IodaDistribution.x )

ecbuild_add_test( TARGET  test_ioda_obsspacecontainer
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsSpaceContainer.x
                  ARGS    "testinput/iodatest_obsspace_container.yml"
                  DEPENDS test_ObsSpaceContainer.x )

ecbuild_add_test( TARGET  test_ioda_oops_obsspace
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsSpace.x
                  ARGS    "testinput/iodatest.yml"
                  DEPENDS test_ObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_obsspace
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_obsspace.yml"
                  DEPENDS test_IodaObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_obsspace_mpi
                  MPI     2
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_obsspace.yml"
                  DEPENDS test_IodaObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_obsspace_marine
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_obsspace_marine.yml"
                  DEPENDS test_IodaObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_fileio
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaIO.x
                  ARGS    "testinput/iodatest_fileio.yml"
                  DEPENDS test_IodaIO.x )

ecbuild_add_test( TARGET  test_ioda_localobsspace
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsSpace.x
                  ARGS    "testinput/iodatest_localobsspace.yml"
                  DEPENDS test_IodaObsSpace.x )

if (HAVE_ODB_API)

ecbuild_add_test( TARGET  test_ioda_oops_obsspace_odb
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsSpace.x
                  ARGS    "testinput/iodatest_odb.json"
                  DEPENDS test_ObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_oops_obsvector_odb
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsVector.x
                  ARGS    "testinput/iodatest_odb.json"
                  DEPENDS test_ObsVector.x )

ecbuild_add_test( TARGET  test_ioda_obsvector_odb
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaObsVector.x
                  ARGS    "testinput/iodatest_obsvector_odb.yml"
                  DEPENDS test_IodaObsVector.x )

ecbuild_add_test( TARGET  test_ioda_fileio_odb_api
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_IodaIO.x
                  ARGS    "testinput/iodatest_fileio_odb_api.yml"
                  DEPENDS test_IodaIO.x )

endif (HAVE_ODB_API)

if (HAVE_ODB)

ecbuild_add_test( TARGET  test_ioda_oops_obsspace_odb1
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsSpace.x
                  ARGS    "testinput/iodatest_odb1.json"
                  DEPENDS test_ObsSpace.x )

ecbuild_add_test( TARGET  test_ioda_oops_obsvector_odb1
                  COMMAND ${CMAKE_BINARY_DIR}/bin/test_ObsVector.x
                  ARGS    "testinput/iodatest_odb1.json"
                  DEPENDS test_ObsVector.x )

endif (HAVE_ODB)

if( HAVE_FCTEST )
add_fctest( TARGET  test_ioda_obsspace_fortran
            SOURCES ioda/obsspace.F90
            ARGS    --config testinput/iodatest_obsspace_fortran.yml
            CONDITION HAVE_FCTEST
            LINKER_LANGUAGE Fortran
            LIBS    ioda )
endif()

