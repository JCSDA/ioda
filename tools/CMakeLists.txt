#List of programs to install to build-tree
set(_programs
    ioda_cpplint.py
    check_ioda_nc.py
)

foreach(_f IN LISTS _programs)
  execute_process( COMMAND ${CMAKE_COMMAND} -E copy
     ${PROJECT_SOURCE_DIR}/tools/${_f} ${CMAKE_BINARY_DIR}/bin/${_f} )
endforeach()
