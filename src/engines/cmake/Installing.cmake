# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
	#set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	cmake_host_system_information(RESULT OSprops QUERY
		OS_NAME OS_RELEASE OS_VERSION OS_PLATFORM)
	list(GET OSprops 0 iOS_NAME)
	if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Linux")
		set(CMAKE_INSTALL_RPATH "\\\$ORIGIN/../lib")
	elseif("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "CYGWIN")
		set(CMAKE_INSTALL_RPATH "\\\$ORIGIN/../lib")
	elseif("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
		set(CMAKE_INSTALL_RPATH "@loader_path")
	elseif("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Windows")
	else()
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

		message(STATUS "Cannot figure out how to set the install RPATH for platform ${CMAKE_HOST_SYSTEM_NAME}. Defaulting to ${CMAKE_INSTALL_RPATH}.")
	endif()

endif("${isSystemDir}" STREQUAL "-1")

#message("CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH}")


