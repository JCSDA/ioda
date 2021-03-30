# (C) Copyright 2020-2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(GNUInstallDirs)
include(Targets)

list(APPEND SRCS_IODA
	include/ioda/defs.h
	include/ioda/Attributes/Attribute.h
	include/ioda/Attributes/Attribute_Creator.h
	include/ioda/Python/Att_ext.h
	include/ioda/Engines/Capabilities.h
	#include/ioda/Copying.h
	include/ioda/Misc/Dimensions.h
	include/ioda/Misc/Eigen_Compat.h
	include/ioda/Misc/MergeMethods.h
	include/ioda/Misc/SFuncs.h
	include/ioda/Variables/Fill.h
	src/ioda/Fill.cpp
	include/ioda/Variables/FillPolicy.h
	include/ioda/Group.h
	include/ioda/Layout.h
	include/ioda/Attributes/Has_Attributes.h
	include/ioda/Variables/Has_Variables.h
	include/ioda/Types/Marshalling.h
	include/ioda/ObsGroup.h
	include/ioda/Variables/Selection.h
	include/ioda/Types/Type.h
	include/ioda/Types/Type_Provider.h
	include/ioda/Variables/Variable.h
	include/ioda/Python/Var_ext.h
	private/ioda/Layouts/Layout_ObsGroup.h
	private/ioda/Layouts/Layout_ObsGroup_ODB.h
	private/ioda/Layouts/Layout_Utils.h
	src/ioda/Attribute.cpp
	src/ioda/Attribute_Creator.cpp
	#src/ioda/Copying.cpp
	src/ioda/Has_Attributes.cpp
	src/ioda/Has_Variables.cpp
	src/ioda/Group.cpp
	src/ioda/ObsGroup.cpp
	src/ioda/Selection.cpp
	src/ioda/SFuncs.cpp
	src/ioda/Type.cpp
	src/ioda/Type_Provider.cpp
	src/ioda/Variable.cpp
	src/ioda/Layouts/Layout.cpp
	src/ioda/Layouts/Layout_ObsGroup.cpp
	src/ioda/Layouts/Layout_ObsGroup_ODB.cpp
	src/ioda/Layouts/Layout_Utils.cpp
  )
list(APPEND SRCS_IODA_TYPES
	include/ioda/Types/Complex.h
	include/ioda/Types/Sample.h
	)
source_group("Custom Types" FILES ${SRCS_IODA_TYPES})
list(APPEND SRCS_ENGINES_TOP
	include/ioda/Engines/Factory.h
	src/ioda/Engines/Factory.cpp)

list(APPEND SRCS_ENGINES_HH 
	include/ioda/Engines/HH.h
	src/ioda/Engines/HH/HH.cpp

	src/ioda/Engines/HH/HH/Handles.h
	src/ioda/Engines/HH/Handles.cpp

	src/ioda/Engines/HH/HH-attributes.cpp
	src/ioda/Engines/HH/HH/HH-attributes.h
	src/ioda/Engines/HH/HH-Filters.cpp
	src/ioda/Engines/HH/HH/HH-Filters.h
	src/ioda/Engines/HH/HH-groups.cpp
	src/ioda/Engines/HH/HH/HH-groups.h
	src/ioda/Engines/HH/HH-hasattributes.cpp
	src/ioda/Engines/HH/HH/HH-hasattributes.h
	src/ioda/Engines/HH/HH-hasvariables.cpp
	src/ioda/Engines/HH/HH/HH-hasvariables.h
	src/ioda/Engines/HH/HH-types.cpp
	src/ioda/Engines/HH/HH/HH-types.h
	src/ioda/Engines/HH/HH-variablecreation.cpp
	src/ioda/Engines/HH/HH/HH-variablecreation.h
	src/ioda/Engines/HH/HH-variables.cpp
	src/ioda/Engines/HH/HH/HH-variables.h
	)

list(APPEND SRCS_ENGINES_OBS_STORE 
	include/ioda/Engines/ObsStore.h
	src/ioda/Engines/ObsStore/ObsStore.cpp
	src/ioda/Engines/ObsStore/ObsStore-attributes.cpp
	src/ioda/Engines/ObsStore/ObsStore-selection.cpp
	src/ioda/Engines/ObsStore/ObsStore-types.cpp
	src/ioda/Engines/ObsStore/ObsStore-variables.cpp
	src/ioda/Engines/ObsStore/Attributes.hpp
	src/ioda/Engines/ObsStore/VarAttrStore.hpp
	src/ioda/Engines/ObsStore/Group.hpp
	src/ioda/Engines/ObsStore/Selection.hpp
	src/ioda/Engines/ObsStore/Types.hpp
	src/ioda/Engines/ObsStore/Variables.hpp
	src/ioda/Engines/ObsStore/ObsStore-attributes.h
	src/ioda/Engines/ObsStore/ObsStore-groups.h
	src/ioda/Engines/ObsStore/ObsStore-selection.h
	src/ioda/Engines/ObsStore/ObsStore-types.h
	src/ioda/Engines/ObsStore/ObsStore-variables.h
	src/ioda/Engines/ObsStore/Attributes.cpp
	src/ioda/Engines/ObsStore/Group.cpp
	src/ioda/Engines/ObsStore/Selection.cpp
	src/ioda/Engines/ObsStore/Variables.cpp
	)

list(APPEND SRCS_C_BINDINGS
	include/ioda/C/Attribute_c.h
	include/ioda/C/c_binding_macros.h
	include/ioda/C/Dimensions_c.h
	include/ioda/C/Engines_c.h
	include/ioda/C/Group_c.h
	include/ioda/C/Has_Attributes_c.h
	include/ioda/C/Has_Variables_c.h
	include/ioda/C/ioda_c.h
	include/ioda/C/String_c.h
	include/ioda/C/Variable_c.h
	include/ioda/C/Variable_Creation_Parameters_c.h
	src/ioda/C/structs_c.h
	src/ioda/C/Attribute_c.cpp
	src/ioda/C/Dimensions_c.cpp
	src/ioda/C/Engines_c.cpp
	src/ioda/C/Group_c.cpp
	src/ioda/C/Has_Attributes_c.cpp
	src/ioda/C/ioda_c.cpp
	src/ioda/C/String_c.cpp
	src/ioda/C/Variable_Creation_Parameters_c.cpp
	src/ioda/C/Has_Variables_c.cpp
	src/ioda/C/Variable_c.cpp
	)

source_group("C Bindings" FILES ${SRCS_C_BINDINGS})
source_group("Engines" FILES ${SRCS_ENGINES_TOP})
source_group("Engines\\HH" FILES ${SRCS_ENGINES_HH})
source_group("Engines\\ObsStore" FILES ${SRCS_ENGINES_OBS_STORE})
add_library(ioda_engines SHARED ${SRCS_IODA} ${SRCS_C_BINDINGS} ${SRCS_ENGINES_HH} ${SRCS_ENGINES_TOP} ${SRCS_IODA_TYPES} ${SRCS_ENGINES_OBS_STORE})

if(DEFINED HDF5_C_DEFINITIONS)
	target_compile_options(ioda_engines PRIVATE "${HDF5_C_DEFINITIONS}")
endif()
if(DEFINED HDF5_C_INCLUDE_DIRS)
	target_include_directories(ioda_engines SYSTEM PUBLIC ${HDF5_C_INCLUDE_DIRS})
endif()
if(DEFINED HDF5_C_LIBRARIES)
	target_link_libraries(ioda_engines PUBLIC ${HDF5_C_LIBRARIES})
endif()
if(DEFINED HDF5_HL_LIBRARIES)
	target_link_libraries(ioda_engines PUBLIC ${HDF5_HL_LIBRARIES})
endif()
if(HDF5_IS_PARALLEL)
	target_link_libraries(ioda_engines PUBLIC MPI::MPI_C MPI::MPI_CXX)
endif()

target_link_libraries(ioda_engines PUBLIC eckit)
target_link_libraries(ioda_engines PUBLIC gsl::gsl-lite)
target_link_libraries(ioda_engines PUBLIC Eigen3::Eigen)

addlib(ioda_engines SHARED ioda ioda_engines-targets)
set_property(TARGET ioda_engines PROPERTY VERSION ${IODA_ENGINES_VERSION})
set_property(TARGET ioda_engines PROPERTY SOVERSION ${IODA_ENGINES_TWEAK})
set_property(TARGET ioda_engines PROPERTY
	INTERFACE_ioda_engines_MAJOR_VERSION ${IODA_ENGINES_MAJOR})
set_property(TARGET ioda_engines APPEND PROPERTY
	COMPATIBLE_INTERFACE_STRING ioda_engines_MAJOR_VERSION
	)


target_include_directories(ioda_engines PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/private>
	)

target_include_directories(ioda_engines PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>	
	)

INSTALL(DIRECTORY include/ioda
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT Headers)

if (BUILD_PYTHON_BINDINGS)
	add_subdirectory(python)
endif()

if (IODA_ENGINES_IS_PROJECT)
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(
		"${PROJECT_BINARY_DIR}/ioda_engines/ioda_enginesConfigVersion.cmake"
		VERSION ${IODA_ENGINES_VERSION}
		COMPATIBILITY AnyNewerVersion
		)

	export(EXPORT ioda_engines-targets
		FILE "${PROJECT_BINARY_DIR}/ioda_engines-targets.cmake"
		NAMESPACE ioda_engines::
		)

	configure_file(${PROJECT_SOURCE_DIR}/cmake/ioda_engines-config.cmake
		"${PROJECT_BINARY_DIR}/ioda_engines-config.cmake"
		COPYONLY
		)

	set(ConfigPackageLocation lib/cmake/ioda_engines)
	install(EXPORT ioda_engines-targets
		FILE ioda_engines-targets.cmake
		NAMESPACE ioda_engines::
		DESTINATION ${ConfigPackageLocation}
		)

	install(
		FILES
			"${PROJECT_SOURCE_DIR}/cmake/ioda_engines-config.cmake"
			"${PROJECT_BINARY_DIR}/ioda_engines/ioda_enginesConfigVersion.cmake"
		DESTINATION ${ConfigPackageLocation}
		COMPONENT Headers
		)

	# Install the share/ directory
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/share/ioda_engines DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
else()
	# This is a subproject of ioda, and the targets need to be exported to
	# the ioda packaging.
	list(APPEND IodaEnginesTargets
		ioda_engines
		)

	install(TARGETS ${IodaEnginesTargets}
		EXPORT  ${PROJECT_NAME}-targets
		RUNTIME DESTINATION ${INSTALL_BIN_DIR}
		LIBRARY DESTINATION ${INSTALL_LIB_DIR}
		ARCHIVE DESTINATION ${INSTALL_LIB_DIR} )
endif()

