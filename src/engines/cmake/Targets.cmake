# (C) Copyright 2020 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

macro(ApplyBaseSettings tgt)

target_compile_definitions(${tgt} PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
        # remove unnecessary warnings about unchecked iterators
        _SCL_SECURE_NO_WARNINGS
		_CRT_SECURE_NO_WARNINGS
        # remove deprecation warnings about std::uncaught_exception() (from catch)
        _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
	# Report the correct C++ version
	/Zc:__cplusplus
	# Multiprocessor compilation
	#/MP
	>
	)

	# GCC-specific options
	target_compile_options(${tgt} PUBLIC
		$<$<CXX_COMPILER_ID:GNU>:
		# Turn off GCC 4.4 ABI warning involving unions of long double.
		-Wno-psabi
		>
		)

	set_property(TARGET ${tgt} PROPERTY CXX_STANDARD 14) # When updating, also see below target_compile_features cxx_std_14.

	set_target_properties( ${tgt}
		PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
		)
endmacro(ApplyBaseSettings)

macro(AddLib)
	set(libname "${ARGV0}")
	if ("" STREQUAL "${ARGV1}")
		set(isshared "STATIC")
	else()
		set(isshared "${ARGV1}")
	endif()
	if ("" STREQUAL "${ARGV2}")
		set(libshared ${libname})
	else()
		set(libshared ${ARGV2})
	endif()
	if ("" STREQUAL "${ARGV3}")
		set(exporttgt ioda_engines-targets)
	else()
		set(exporttgt ${ARGV3})
	endif()


	ApplyBaseSettings(${libname})

	set_target_properties( ${libname} PROPERTIES FOLDER "Libs")
	target_compile_definitions(${libname} PRIVATE ${libshared}_EXPORTING)
	target_compile_definitions(${libname} PUBLIC ${libshared}_SHARED=$<STREQUAL:${isshared},SHARED>)
	target_compile_features(${libname} PUBLIC cxx_std_14) # C++14 or greater is needed to parse the headers.

	INSTALL(TARGETS ${libname}
		EXPORT ${exporttgt}
		#RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # See NSIS below
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		COMPONENT Libraries)
	# NSIS bug. Have to do this twice.
	if (WIN32 AND NOT CYGWIN)
		if("SHARED" STREQUAL "${libshared}")
			# Don't export this one for NSIS.
			INSTALL(TARGETS ${libname} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
		endif()
	endif()
endmacro(AddLib)

macro(AddPyLib libname pyname pybasename)

    ApplyBaseSettings(${libname})
    target_compile_features(${libname} PUBLIC cxx_std_14) # C++14 or greater is needed to parse the headers.

    set_target_properties( ${libname} PROPERTIES FOLDER "Libs/Python/${pybasename}")
	set_target_properties( ${libname} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${pybasename}/${pyname}"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${pybasename}/${pyname}"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${pybasename}/${pyname}"
		INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}"
		)

	INSTALL(TARGETS ${libname}
		EXPORT ioda_engines-targets
		#RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # See NSIS below
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${pybasename}/${pyname}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${pybasename}/${pyname}
		COMPONENT Libraries)
	# NSIS bug. Have to do this twice.
	if (WIN32 AND NOT CYGWIN)
		if("SHARED" STREQUAL "${libshared}")
			# Don't export this one for NSIS.
			INSTALL(TARGETS ${libname} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${pybasename}/${pyname})
		endif()
	endif()
endmacro(AddPyLib)

macro(AddApp appname)
	ApplyBaseSettings(${appname})
	set_target_properties( ${appname} PROPERTIES FOLDER "Apps")
	# All apps currently are unit tests. No need to install.
	#INSTALL(TARGETS ${appname} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endmacro(AddApp)
