#
# (C) Copyright 2017-2022 UCAR & Met Office.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

## Files used by both test and in non-test executables

set(
  testinput_files
  ioda/yaml/iodatest_odb_abiclr.yaml
  ioda/yaml/iodatest_odb_ahiclr.yaml
  ioda/yaml/iodatest_odb_aircraft.yaml
  ioda/yaml/iodatest_odb_aircraft_write.yaml
  ioda/yaml/iodatest_odb_airs.yaml
  ioda/yaml/iodatest_odb_altimeter.yaml
  ioda/yaml/iodatest_odb_amsr.yaml
  ioda/yaml/iodatest_odb_amsub.yaml
  ioda/yaml/iodatest_odb_aod.yaml
  ioda/yaml/iodatest_odb_aod_write.yaml
  ioda/yaml/iodatest_odb_aod_write_testdatasets.yaml
  ioda/yaml/iodatest_odb_atms.yaml
  ioda/yaml/iodatest_odb_atms_write.yaml
  ioda/yaml/iodatest_odb_atovs.yaml
  ioda/yaml/iodatest_odb_atovs_write.yaml
  ioda/yaml/iodatest_odb_cris.yaml
  ioda/yaml/iodatest_odb_crisfsr.yaml
  ioda/yaml/iodatest_odb_geocloud.yaml
  ioda/yaml/iodatest_odb_gmihigh.yaml
  ioda/yaml/iodatest_odb_gmihigh_write.yaml
  ioda/yaml/iodatest_odb_gmilow.yaml
  ioda/yaml/iodatest_odb_gnssro.yaml
  ioda/yaml/iodatest_odb_gnssro_profile.yaml
  ioda/yaml/iodatest_odb_groundgps.yaml
  ioda/yaml/iodatest_odb_hirasfsr.yaml
  ioda/yaml/iodatest_odb_hloswind.yaml
  ioda/yaml/iodatest_odb_iasi.yaml
  ioda/yaml/iodatest_odb_iasi_write.yaml
  ioda/yaml/iodatest_odb_mwri.yaml
  ioda/yaml/iodatest_odb_mwsfy3.yaml
  ioda/yaml/iodatest_odb_oceancolour.yaml
  ioda/yaml/iodatest_odb_oceansound.yaml
  ioda/yaml/iodatest_odb_oceansound_truncate_profiles.yaml
  ioda/yaml/iodatest_odb_satsst.yaml
  ioda/yaml/iodatest_odb_sattcwv.yaml
  ioda/yaml/iodatest_odb_satwind.yaml
  ioda/yaml/iodatest_odb_scatwind.yaml
  ioda/yaml/iodatest_odb_seaice.yaml
  ioda/yaml/iodatest_odb_seviriclr.yaml
  ioda/yaml/iodatest_odb_sonde.yaml
  ioda/yaml/iodatest_odb_sonde_history.yaml
  ioda/yaml/iodatest_odb_sonde_mixed.yaml
  ioda/yaml/iodatest_odb_sonde_time_displacement.yaml
  ioda/yaml/iodatest_odb_sonde_truncate_profiles.yaml
  ioda/yaml/iodatest_odb_sonde_write.yaml
  ioda/yaml/iodatest_odb_sonde_write_mixed.yaml
  ioda/yaml/iodatest_odb_ssmis.yaml
  ioda/yaml/iodatest_odb_surface.yaml
  ioda/yaml/iodatest_odb_surfacecloud.yaml
  ioda/yaml/iodatest_odb_surfacesst.yaml
  ioda/yaml/odb_default_name_map.yaml
  ioda/yaml/odb_aircraft_name_map.yaml
  ioda/yaml/odb_gnssro_name_map.yaml
  ioda/yaml/odb_gnssro_profile_name_map.yaml
  ioda/yaml/odb_oceansound_name_map.yaml
  ioda/yaml/odb_radiance_name_map.yaml
  ioda/yaml/odb_sonde_name_map.yaml
  ioda/yaml/odb_surface_name_map.yaml
  ioda/yaml/odb_surfacesst_name_map.yaml
)

set(testinput_dest "${CMAKE_CURRENT_BINARY_DIR}/test/testinput/")

file(INSTALL ${testinput_files} DESTINATION "${testinput_dest}")
file(
  INSTALL "ioda/yaml/validation/ObsSpace.yaml"
  DESTINATION "${testinput_dest}/validation/"
)
set(IODA_YAML_ROOT "${testinput_dest}" PARENT_SCOPE)
install(DIRECTORY ioda TYPE DATA COMPONENT share)
