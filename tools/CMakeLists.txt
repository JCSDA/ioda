# Create Data directory for test input data
list( APPEND programs
  cpplint.py
  check_ioda_nc.py
)

CONF_TARGETS_DEPS( "${programs}"
                   ${CMAKE_CURRENT_SOURCE_DIR}
                   ${CMAKE_BINARY_DIR}/bin
                   bin_tools_programs_deps
)

add_custom_target( bin_tools_programs ALL
  COMMAND chmod +x ${bin_tools_programs_deps}
  DEPENDS ${bin_tools_programs_deps}
)
