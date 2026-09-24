# Create a list of all the modules within the package
list( APPEND pyiodautils_files
  __init__.py
  file_merge.py
  )

# Set the path to the build install area for pyiodautils
set(pyver python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})
set ( PYIODAUTILS_BUILD_LIBDIR ${CMAKE_BINARY_DIR}/lib/${pyver}/pyiodautils )
set ( PYIODAUTILS_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${pyver}/pyiodautils )

# Set dependencies for the module target
foreach( FILENAME ${pyiodautils_files} )
    set ( SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} )
    set ( DEST_FILE ${PYIODAUTILS_BUILD_LIBDIR}/${FILENAME} )
    list ( APPEND pyiodautils_file_deps ${DEST_FILE})
    configure_file( ${SOURCE_FILE} ${DEST_FILE} COPYONLY )
endforeach()

# Set install for the build area
install( FILES ${pyiodautils_file_deps} DESTINATION ${PYIODAUTILS_BUILD_LIBDIR} )

# Run pycodestyle on the source files
set ( PYCODESTYLE_CFG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../.." )

ecbuild_add_test( TARGET  ${PROJECT_NAME}_pyiodautils_coding_norms
                  TYPE    SCRIPT
                  COMMAND ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}_pylint.sh
                  ARGS    ${CMAKE_CURRENT_SOURCE_DIR} ${PYCODESTYLE_CFG_PATH} )

# Set install area for the `make install` target
install( FILES ${pyiodautils_files} DESTINATION ${PYIODAUTILS_INSTALL_LIBDIR} )
