include(Targets)

set(HH_MAJOR 0)
set(HH_MINOR 7)
set(HH_REVISION 2)
set(HH_VERSION ${HH_MAJOR}.${HH_MINOR}.${HH_REVISION})

set(DIRS_HH
	include/HH/Attributes.hpp
	include/HH/Datasets.hpp
	include/HH/defs.hpp
	include/HH/Errors.hpp
	include/HH/Files.hpp
	include/HH/Funcs.hpp
	include/HH/Groups.hpp
	include/HH/Handles.hpp
	include/HH/Handles_HDF.hpp
	include/HH/Handles_Valid.hpp
	include/HH/HH.hpp
	include/HH/PropertyLists.hpp
	include/HH/Tags.hpp
	include/HH/Types.hpp
	include/HH/Types/Complex.hpp
	src/Attributes.cpp
	src/Datasets.cpp
	src/Files.cpp
	src/Funcs.cpp
	src/Groups.cpp
	src/Handles.cpp
	src/Types.cpp
	)
add_library(HH SHARED ${DIRS_HH})

target_include_directories(HH SYSTEM PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${INSTALL_INCLUDE_DIR}>	
	)
if(DEFINED HDF5_C_DEFINITIONS) # Buggy because CMake wants to prepend -D.
	target_compile_options(HH PRIVATE "${HDF5_C_DEFINITIONS}")
endif()
if(DEFINED HDF5_C_INCLUDE_DIRS)
	target_include_directories(HH SYSTEM PUBLIC ${HDF5_C_INCLUDE_DIRS})
endif()
if(DEFINED HDF5_C_LIBRARIES)
	target_link_libraries(HH PUBLIC ${HDF5_C_LIBRARIES})
endif()
if(DEFINED HDF5_HL_LIBRARIES)
	target_link_libraries(HH PUBLIC ${HDF5_HL_LIBRARIES})
endif()
if(HDF5_IS_PARALLEL)
	target_link_libraries(HH PUBLIC MPI::MPI_C MPI::MPI_CXX)
endif()

target_link_libraries(HH PUBLIC gsl::gsl-lite)
addlib(HH SHARED HH ioda_engines-targets)

INSTALL(DIRECTORY include/HH
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT Headers)


