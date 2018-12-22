include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

list( APPEND ioda_src_files

ncdiag/ncd_kinds.F90
ncdiag/m_distribution.f90
ncdiag/m_diag_conv.f90
ncdiag/m_diag_aircft.f90
ncdiag/m_diag_raob.f90
ncdiag/nc_diag_fson.f90
ncdiag/nc_diag_read_mod.F90
ncdiag/nc_diag_res.f90
ncdiag/nc_diag_write_mod.F90
ncdiag/ncdc_cli_process.F90
ncdiag/ncdc_climsg.F90
ncdiag/ncdc_data.F90
ncdiag/ncdc_dims.F90
ncdiag/ncdc_metadata.F90
ncdiag/ncdc_realloc.F90
ncdiag/ncdc_state.F90
ncdiag/ncdc_types.f90
ncdiag/ncdc_vars.F90
ncdiag/ncdf_path_m.f90
ncdiag/ncdf_string_m.f90
ncdiag/ncdf_value_m.f90
ncdiag/ncdr_alloc_assert.f90
ncdiag/ncdr_attrs.f90
ncdiag/ncdr_attrs_fetch.f90
ncdiag/ncdr_check.f90
ncdiag/ncdr_climsg.F90
ncdiag/ncdr_dims.f90
ncdiag/ncdr_global_attrs.f90
ncdiag/ncdr_global_attrs_fetch.f90
ncdiag/ncdr_realloc_mod.F90
ncdiag/ncdr_state.f90
ncdiag/ncdr_types.f90
ncdiag/ncdr_vars.f90
ncdiag/ncdr_vars_fetch.f90
ncdiag/ncdres_climsg.F90
ncdiag/ncdw_chaninfo.F90
ncdiag/ncdw_ciresize.F90
ncdiag/ncdw_climsg.F90
ncdiag/ncdw_data2d.F90
ncdiag/ncdw_dresize.F90
ncdiag/ncdw_lheader.F90
ncdiag/ncdw_metadata.F90
ncdiag/ncdw_mresize.F90
ncdiag/ncdw_realloc.F90
ncdiag/ncdw_state.f90
ncdiag/ncdw_strarrutils.F90
ncdiag/ncdw_types.F90
ncdiag/ncdw_varattr.F90
ncdiag/netcdf_unlimdims.F90
ncdiag/read_diag.f90
ncdiag/read_aod_diag.f90

ioda/missingValue.h
ioda/ObsDataVector.h
ioda/ObsSpace.cc
ioda/ObsSpace.h
ioda/ObsVector.cc
ioda/ObsVector.h
ioda/obsspace_f.cc
ioda/obsspace_f.h
ioda/obsspace_mod.F90
ioda/random_f.cc
ioda/random_f.h
ioda/random_vectors_mod.F90
ioda/twindow_utils_mod.F90

database/MultiIndexContainer.cc
database/MultiIndexContainer.h

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

