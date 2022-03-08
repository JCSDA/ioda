
list( APPEND ioda_src_files
IodaTrait.h
ObsDataVector.h
ObsSpace.cc
ObsSpace.h
ObsSpaceParameters.cc
ObsSpaceParameters.h
ObsVector.cc
ObsVector.h

core/FileFormat.cc
core/FileFormat.h
core/IodaUtils.cc
core/IodaUtils.h
core/obsspace_f.cc
core/obsspace_f.h
core/ParameterTraitsFileFormat.cc
core/ParameterTraitsFileFormat.h
core/ParameterTraitsObsDtype.cc
core/ParameterTraitsObsDtype.h

distribution/Accumulator.h
distribution/AtlasDistribution.cc
distribution/AtlasDistribution.h
distribution/Distribution.cc
distribution/Distribution.h
distribution/DistributionFactory.cc
distribution/DistributionFactory.h
distribution/DistributionParametersBase.h
distribution/DistributionUtils.cc
distribution/DistributionUtils.h
distribution/InefficientDistribution.cc
distribution/InefficientDistribution.h
distribution/InefficientDistributionAccumulator.h
distribution/GeneralDistributionAccumulator.h
distribution/Halo.cc
distribution/Halo.h
distribution/PairOfDistributions.cc
distribution/PairOfDistributions.h
distribution/PairOfDistributionsAccumulator.h
distribution/ReplicaOfGeneralDistribution.cc
distribution/ReplicaOfGeneralDistribution.h
distribution/ReplicaOfNonoverlappingDistribution.cc
distribution/ReplicaOfNonoverlappingDistribution.h
distribution/NonoverlappingDistribution.cc
distribution/NonoverlappingDistribution.h
distribution/NonoverlappingDistributionAccumulator.h
distribution/RoundRobin.cc
distribution/RoundRobin.h

io/ObsFrame.cc
io/ObsFrame.h
io/ObsFrameRead.cc
io/ObsFrameRead.h
io/ObsIo.cc
io/ObsIo.h
io/ObsIoFactory.cc
io/ObsIoFactory.h
io/ObsIoFileCreate.cc
io/ObsIoFileCreate.h
io/ObsIoFileRead.cc
io/ObsIoFileRead.h
io/ObsIoParametersBase.h
io/ObsIoGenerateList.cc
io/ObsIoGenerateList.h
io/ObsIoGenerateRandom.cc
io/ObsIoGenerateRandom.h
io/ObsIoGenerateUtils.cc
io/ObsIoGenerateUtils.h
)

if (IODA_BUILD_LANGUAGE_FORTRAN)
  list( APPEND ioda_src_files obsspace_mod.F90)
endif()

list( APPEND ioda_fortran_interface_includes
obsspace_interface.f
)

include(GNUInstallDirs)
ecbuild_add_library( TARGET ${PROJECT_NAME}
                     SOURCES ${${PROJECT_NAME}_src_files}
                     INSTALL_HEADERS LISTED
                     HEADER_DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
                     LINKER_LANGUAGE CXX )

target_link_libraries( ${PROJECT_NAME} PUBLIC ioda_engines )
target_link_libraries( ${PROJECT_NAME} PUBLIC fckit )
target_link_libraries( ${PROJECT_NAME} PUBLIC ${oops_LIBRARIES} )

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
if (IODA_BUILD_LANGUAGE_FORTRAN)
  set(MODULE_DIR module/${PROJECT_NAME}/${CMAKE_Fortran_COMPILER_ID}/${CMAKE_Fortran_COMPILER_VERSION})
  set_target_properties(${PROJECT_NAME} PROPERTIES Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/${MODULE_DIR})
  install(DIRECTORY ${CMAKE_BINARY_DIR}/${MODULE_DIR}/ DESTINATION ${MODULE_DIR})
  target_include_directories(${PROJECT_NAME} INTERFACE
                                $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${MODULE_DIR}>
                                $<INSTALL_INTERFACE:${MODULE_DIR}>)
endif()

