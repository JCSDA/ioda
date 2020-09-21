
list( APPEND ioda_src_files
IodaTrait.h
ObsDataVector.h
ObsSpace.cc
ObsSpace.h
ObsSpaceParameters.h
ObsVector.cc
ObsVector.h
obsspace_mod.F90
instantiateObsLocFactory.h

core/IodaUtils.cc
core/IodaUtils.h
core/LocalObsSpaceParameters.cc
core/LocalObsSpaceParameters.h
core/ObsLocGC99.cc
core/ObsLocGC99.h
core/ObsData.cc
core/ObsData.h
core/obsspace_f.cc
core/obsspace_f.h

distribution/Distribution.cc
distribution/Distribution.h
distribution/DistributionFactory.cc
distribution/DistributionFactory.h
distribution/InefficientDistribution.cc
distribution/InefficientDistribution.h
distribution/RoundRobin.cc
distribution/RoundRobin.h

io/ObsFile.cc
io/ObsFile.h
io/ObsFrame.cc
io/ObsFrame.h
io/ObsGenerate.cc
io/ObsGenerate.h
io/ObsIo.cc
io/ObsIo.h
io/ObsIoFactory.cc
io/ObsIoFactory.h
)

list( APPEND ioda_fortran_interface_includes
obsspace_interface.f
)

include(GNUInstallDirs)
ecbuild_add_library( TARGET ${PROJECT_NAME}
                     SOURCES ${${PROJECT_NAME}_src_files}
                     INSTALL_HEADERS LISTED
                     HEADER_DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
                     LINKER_LANGUAGE CXX )

target_link_libraries( ${PROJECT_NAME} PUBLIC NetCDF::NetCDF_C )
target_link_libraries( ${PROJECT_NAME} PUBLIC fckit )
target_link_libraries( ${PROJECT_NAME} PUBLIC ioda-engines )
target_link_libraries( ${PROJECT_NAME} PUBLIC ${oops_LIBRARIES} ) #TODO: Change to "oops::oops" once oops adds namespace support

#Configure include directory layout for build-tree to match install-tree
set(BUILD_DIR_INCLUDE_PATH ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/include)
add_custom_target(${PROJECT_NAME}_headers ALL
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR_INCLUDE_PATH}
                    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR} ${BUILD_DIR_INCLUDE_PATH}/${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_headers)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${BUILD_DIR_INCLUDE_PATH}>)

#Fortran file interfaces templates
install(FILES ${ioda_fortran_interface_includes} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ioda)

#Fortran module output directory for build and install interfaces
set(MODULE_DIR module/${PROJECT_NAME}/${CMAKE_Fortran_COMPILER_ID}/${CMAKE_Fortran_COMPILER_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/${MODULE_DIR})
install(DIRECTORY ${CMAKE_BINARY_DIR}/${MODULE_DIR}/ DESTINATION ${MODULE_DIR})
target_include_directories(${PROJECT_NAME} INTERFACE
                                $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${MODULE_DIR}>
                                $<INSTALL_INTERFACE:${MODULE_DIR}>)


# add in the mains executables
add_subdirectory(mains)
