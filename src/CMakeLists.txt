include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

list( APPEND ioda_src_files

ioda/ObsDataVector.h
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
distribution/RoundRobin.cc
distribution/RoundRobin.h

fileio/IodaIO.cc
fileio/IodaIO.h
fileio/IodaIOfactory.cc
fileio/IodaIOfactory.h
fileio/NetcdfIO.cc
fileio/NetcdfIO.h
)

if (HAVE_ODB)
  file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/ddl)
  if ("${CMAKE_PROJECT_NAME}" STREQUAL "ioda")
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddl/queries.sql
                         COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/odb/queries.sql ${CMAKE_CURRENT_BINARY_DIR}/ddl/queries.sql
                       )
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.ddl
                         COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/odb/OOPS.ddl ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.ddl
                       )
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.h ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_Sstatic.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_body.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_desc.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_hdr.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/_odb_glue.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_query.c
                         COMMAND . ${ODB_PATH}/bin/use_odb.sh && unset ODB_COMPILER_FLAGS && ${CMAKE_SOURCE_DIR}/src/odb/OpsScr_sql2c
                         DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.ddl ${CMAKE_CURRENT_BINARY_DIR}/ddl/queries.sql ${CMAKE_SOURCE_DIR}/src/odb/OpsScr_sql2c
                         WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ddl"
                       )
  else ("${CMAKE_PROJECT_NAME}" STREQUAL "ioda")
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddl/queries.sql
                         COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/ioda/src/odb/queries.sql ${CMAKE_CURRENT_BINARY_DIR}/ddl/queries.sql
                       )
    add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.ddl
                         COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/ioda/src/odb/OOPS.ddl ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.ddl
                        )
    if (ODB_PATH)
      add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.h ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_Sstatic.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_body.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_desc.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_hdr.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/_odb_glue.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_query.c
                           COMMAND . ${ODB_PATH}/bin/use_odb.sh && unset ODB_COMPILER_FLAGS && ${CMAKE_SOURCE_DIR}/ioda/src/odb/OpsScr_sql2c
                           DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.ddl ${CMAKE_CURRENT_BINARY_DIR}/ddl/queries.sql ${CMAKE_SOURCE_DIR}/ioda/src/odb/OpsScr_sql2c
                           WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ddl"
                         )
    else (ODB_PATH)
      add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.h ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_Sstatic.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_body.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_desc.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_hdr.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/_odb_glue.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_query.c
                           COMMAND . ${CMAKE_BINARY_DIR}/bin/use_odb.sh && unset ODB_COMPILER_FLAGS && ${CMAKE_SOURCE_DIR}/ioda/src/odb/OpsScr_sql2c
                           DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.ddl ${CMAKE_CURRENT_BINARY_DIR}/ddl/queries.sql ${CMAKE_SOURCE_DIR}/ioda/src/odb/OpsScr_sql2c odb98.x
                           WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ddl"
                         )
  endif (ODB_PATH)
  endif ("${CMAKE_PROJECT_NAME}" STREQUAL "ioda")
  list(APPEND oops_src_files ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS.h ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_Sstatic.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_body.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_desc.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_T_hdr.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/_odb_glue.c ${CMAKE_CURRENT_BINARY_DIR}/ddl/OOPS_query.c )
endif (HAVE_ODB)

if (HAVE_ODB_API)

  list( APPEND ioda_src_files
  odb_api/odb_helper_mod.F90
  fileio/OdbApiIO.cc
  fileio/OdbApiIO.h
  )

  ecbuild_add_executable( TARGET  netcdf_to_odb2
                          SOURCES odb_api/netcdf_to_odb2.F90
                          LIBS    ioda ${IODA_EXTRA_LIBRARIES}
                       )

endif (HAVE_ODB_API)

message(STATUS ${ioda_src_files})
ecbuild_add_library( TARGET   ioda
                     SOURCES  ${ioda_src_files}
                     LIBS     oops ${IODA_EXTRA_LIBRARIES}
                     INSTALL_HEADERS LISTED
                     LINKER_LANGUAGE ${IODA_LINKER_LANGUAGE}
                    )

if (HAVE_ODB_API)
  set_target_properties (ioda PROPERTIES COMPILE_DEFINITIONS "HAVE_ODB_API")
endif (HAVE_ODB_API)

if (HAVE_ODB)

  ecbuild_add_executable( TARGET  netcdf_to_odb
                          SOURCES odb_api/netcdf_to_odb.F90
                          LIBS    ${IODA_EXTRA_LIBRARIES}
                       )


  message(STATUS ${oops_src_files})
  ecbuild_add_library( TARGET   OOPS
                       SOURCES  ${oops_src_files}
                       LIBS     ${ODB_LIBRARY_odb}
                       INSTALL_HEADERS LISTED
                       LINKER_LANGUAGE ${IODA_LINKER_LANGUAGE}
                      )
endif (HAVE_ODB)

# coding norms check
ecbuild_add_test( TARGET ioda_coding_norms
                  TYPE SCRIPT
                  COMMAND cpplint.py
                  ARGS --quiet --recursive ${CMAKE_CURRENT_SOURCE_DIR}
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

