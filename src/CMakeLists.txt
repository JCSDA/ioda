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

#Marine read/write
ncdiag/marine/m_diag_marine_conv_mod.F90

ioda/Fortran.h
ioda/ObsSpace.cc
ioda/ObsSpace.h
ioda/ObsSpace.interface.F90
ioda/ObsVector.cc
ioda/ObsVector.h
ioda/Locations.cc
ioda/Locations.h
ioda/Locations.interface.F90
ioda/ioda_locs_mod.F90
ioda/ioda_obs_vectors.F90
ioda/ioda_obsvar_mod.F90
ioda/ioda_obsdb_mod.F90
ioda/random_f.cc
ioda/random_f.h
ioda/random_vectors_mod.F90

# Marine: Sea-ice fraction
ioda/marine/seaicefraction/ObsSpace.SeaIceFraction.interface.F90
ioda/marine/seaicefraction/ioda_obs_seaicefrac_mod.F90
# Marine: Sea-ice thickness
ioda/marine/seaicethickness/ObsSpace.SeaIceThickness.interface.F90
ioda/marine/seaicethickness/ioda_obs_seaicethick_mod.F90
# Marine: Steric height
ioda/marine/stericheight/ObsSpace.StericHeight.interface.F90
ioda/marine/stericheight/ioda_obs_stericheight_mod.F90
# Marine: Insitu temperature
ioda/marine/insitutemperature/ObsSpace.InsituTemperature.interface.F90
ioda/marine/insitutemperature/ioda_obs_insitutemperature_mod.F90

)



message(STATUS ${ioda_src_files})
ecbuild_add_library( TARGET   ioda
                     SOURCES  ${ioda_src_files}
                     LIBS     oops ${IODA_EXTRA_LIBRARIES}
                     INSTALL_HEADERS LISTED
                     LINKER_LANGUAGE ${IODA_LINKER_LANGUAGE}
                    )

