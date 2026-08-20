#List of programs to install to build-tree
set(_programs
    ioda_cpplint.py
    ioda_pylint.sh
    check_ioda_nc.py
    ioda_compare.sh
    ioda_compare_nc.py
    ioda_compare_obs.py
    ioda_compare_obs_selftest.py
    ioda_compare_odc_with_netcdf.py
    refactor-yaml.py
    retrieval_upgrader.py
    compare-odbs
)

file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
foreach(_f IN LISTS _programs)
  file( COPY_FILE ${PROJECT_SOURCE_DIR}/tools/${_f}
        ${CMAKE_BINARY_DIR}/bin/${_f} ONLY_IF_DIFFERENT )
endforeach()
