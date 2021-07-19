include(Targets)

pybind11_add_module(_ioda_python py_ioda.cpp py_types.cpp py_dimensions.cpp py_attributes.cpp
	py_has_attributes.cpp py_selections.cpp py_variables.cpp py_vcps.cpp py_has_variables.cpp
	py_groups.cpp py_engines.cpp py_dlp.cpp py_scales.cpp py_obsgroup.cpp)
target_link_libraries(_ioda_python PUBLIC ioda_engines Python3::Python)
target_link_libraries(_ioda_python PUBLIC Eigen3::Eigen)
set(pyver python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})
AddPyLib(_ioda_python ioda ${pyver})
target_compile_features(_ioda_python PUBLIC cxx_std_14) # C++14 or greater is needed to parse the headers.

if(APPLE)
	set_target_properties(_ioda_python PROPERTIES
		BUILD_RPATH_USE_ORIGIN TRUE
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH "@loader_path/../../"
		)
else()
	set_target_properties(_ioda_python PROPERTIES
		BUILD_RPATH_USE_ORIGIN TRUE # $ORIGIN
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH "\$ORIGIN/../../"
		)
endif()

add_custom_command(TARGET _ioda_python POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_CURRENT_SOURCE_DIR}/pyioda/ioda/__init__.py $<TARGET_FILE_DIR:_ioda_python>/__init__.py)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pyioda/ioda/__init__.py
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/${pyver}/ioda/
	)

