# (C) Copyright 2020-2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(GNUInstallDirs)

function(GetInstallRpath outvar)
  # The RPATH should be set when installing only if we are NOT installing to a system directory
  # If no RPATH is needed, then outvar is unset.
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)

  if("${isSystemDir}" STREQUAL "-1")
    cmake_host_system_information(RESULT OSprops QUERY
      OS_NAME OS_RELEASE OS_VERSION OS_PLATFORM)
    list(GET OSprops 0 iOS_NAME)
    if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Linux")
      set(rp "\\\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    elseif("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "CYGWIN")
      set(rp "\\\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    elseif("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
      set(rp "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
    elseif("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Windows")
    else()
      set(rp "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
      message(STATUS "Cannot figure out how to set the install RPATH for platform ${CMAKE_HOST_SYSTEM_NAME}. Defaulting to ${rp}.")
    endif()
    set(${outvar} ${rp} PARENT_SCOPE)
  endif("${isSystemDir}" STREQUAL "-1")

endfunction(GetInstallRpath)

macro(ApplyBaseSettings tgt)
    target_compile_definitions(${tgt} PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:
        # remove unnecessary warnings about unchecked iterators
        _SCL_SECURE_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS
        # remove deprecation warnings about std::uncaught_exception() (from catch)
        _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
        >
        )

    target_compile_options(${tgt} PUBLIC
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<COMPILE_LANGUAGE:CXX>>:
            # Turn off GCC 4.4 ABI warning involving unions of long double.
            -Wno-psabi
            >
        $<$<CXX_COMPILER_ID:MSVC>:
            # Report the correct C++ version
            /Zc:__cplusplus
            # Multiprocessor compilation
            #/MP
            >
        )

    set_target_properties( ${tgt} PROPERTIES SKIP_BUILD_RPATH FALSE )
    set_target_properties( ${tgt} PROPERTIES BUILD_WITH_INSTALL_RPATH FALSE )
    set_target_properties( ${tgt} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE )

    GetInstallRpath(rp)
    if ( rp )
        set_target_properties( ${tgt} PROPERTIES INSTALL_RPATH ${rp} )
    endif()

    # Used for example apps
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
        set(exporttgt ${PROJECT_NAME}-targets)
    else()
        set(exporttgt ${ARGV3})
    endif()


    ApplyBaseSettings(${libname})

    set_target_properties( ${libname} PROPERTIES FOLDER "Libs")
    target_compile_definitions(${libname} PRIVATE ${libshared}_EXPORTING)
    target_compile_definitions(${libname} PUBLIC ${libshared}_SHARED=$<STREQUAL:${isshared},SHARED>)
endmacro(AddLib)

macro(AddPyLib libname pyname pybasename)

    ApplyBaseSettings(${libname})
    set_target_properties( ${libname} PROPERTIES FOLDER "Libs/Python/${pybasename}")
    set_target_properties( ${libname} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${pybasename}/pyioda/${pyname}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${pybasename}/pyioda/${pyname}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${pybasename}/pyioda/${pyname}"
        INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}"
        )
    export( TARGETS ${libname} APPEND FILE "${PROJECT_TARGETS_FILE}" )
    install( TARGETS ${libname}
        EXPORT ${PROJECT_NAME}-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${pybasename}/pyioda/${pyname}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${pybasename}/pyioda/${pyname}
        COMPONENT Python)
endmacro(AddPyLib)

macro(AddApp appname)
    ApplyBaseSettings(${appname})
    set_target_properties( ${appname} PROPERTIES FOLDER "Apps")
endmacro(AddApp)

