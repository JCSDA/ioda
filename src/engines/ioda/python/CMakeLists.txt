include(Targets)

# We cannot call ecbuild_add_library here since it conflicts with pybind11_add_module.
pybind11_add_module(_ioda_python py_ioda.cpp py_types.cpp py_dimensions.cpp py_attributes.cpp
	py_has_attributes.cpp py_selections.cpp py_variables.cpp py_vcps.cpp py_has_variables.cpp
	py_groups.cpp py_engines.cpp py_dlp.cpp py_scales.cpp py_obsgroup.cpp)
target_link_libraries(_ioda_python PUBLIC ioda_engines Python3::Module)
target_link_libraries(_ioda_python PUBLIC Eigen3::Eigen)
set(pyver python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})

# TODO: Make an alias to ioda::Python

# AddPyLib will set the target file for _ioda_python to the subdirectory:
#
#   lib/${pyver}/pyioda/ioda/_ioda_python.*  # .* is the appropriate system suffix for
#                                            # a shared library
#
# underneath the install prefix (for both package install and build/test install). We've
# got two python modules we want to install, ioda and obs_space, and they go in the
# subdirectory:
#
#   lib/${pyver}/pyioda
#
# which is one directory up from the generator TARGET_FILE_DIR for the _ioda_python target.
# Note that TARGET_FILE_DIR is like running dirname on a file path in the shell.
AddPyLib(_ioda_python ioda ${pyver})
target_compile_features(_ioda_python PUBLIC cxx_std_14) # C++14 or greater is needed to parse the headers.

# Need to set RPATH to directory where libioda et al. got installed which is three up
# from where _ioda_python.* got installed.
if(APPLE)
	set_target_properties(_ioda_python PROPERTIES
		BUILD_RPATH_USE_ORIGIN TRUE
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH "@loader_path/../../../"
		)
else()
	set_target_properties(_ioda_python PROPERTIES
		BUILD_RPATH_USE_ORIGIN TRUE # $ORIGIN
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH "\$ORIGIN/../../../"
		)
endif()

set(PYIODA_DIR "$<TARGET_FILE_DIR:_ioda_python>/..")
add_custom_command(TARGET _ioda_python POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/pyioda/ioda/__init__.py ${PYIODA_DIR}/ioda/__init__.py)

add_custom_command(TARGET _ioda_python POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/pyioda/ioda_obs_space/__init__.py ${PYIODA_DIR}/ioda_obs_space/__init__.py)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pyioda/ioda/__init__.py
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/${pyver}/pyioda/ioda/
        COMPONENT Python
	)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pyioda/ioda_obs_space/__init__.py
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/${pyver}/pyioda/ioda_obs_space/
        COMPONENT Python
	)

