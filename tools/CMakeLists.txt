
# Create Data directory for test input data
list( APPEND test_files
  cpplint.py
  check_ioda_nc.py
)

foreach(FILENAME ${test_files})
  execute_process( COMMAND ${CMAKE_COMMAND} -E copy
     ${PROJECT_SOURCE_DIR}/tools/${FILENAME}
     ${CMAKE_BINARY_DIR}/bin/${FILENAME} )
endforeach(FILENAME)

