# Find udunits

find_path (
	udunits_INCLUDE_DIR
	udunits2.h
	HINTS $ENV{UDUNITS2_INCLUDE_DIRS} ${UDUNITS2_INCLUDE_DIRS}
		$ENV{UDUNITS2_PATH} ${UDUNITS2_PATH}
		$ENV{UDUNITS2_ROOT} ${UDUNITS2_ROOT}
	DOC "Path to udunits2.h"
	)
find_library(udunits_SHARED_LIB
	NAMES udunits2 udunits
	HINTS $ENV{UDUNITS2_LIBRARIES} ${UDUNITS2_LIBRARIES}
		$ENV{UDUNITS2_PATH} ${UDUNITS2_PATH}
		$ENV{UDUNITS2_ROOT} ${UDUNITS2_ROOT}
	DOC "Path to libudunits"
	)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (udunits DEFAULT_MSG udunits_SHARED_LIB udunits_INCLUDE_DIR)

mark_as_advanced (udunits_SHARED_LIB udunits_INCLUDE_DIR)

add_library(udunits_lib UNKNOWN IMPORTED)
set_property(TARGET udunits_lib PROPERTY IMPORTED_LOCATION ${udunits_SHARED_LIB})
target_include_directories(udunits_lib INTERFACE ${udunits_INCLUDE_DIR})

add_library(udunits::udunits ALIAS udunits_lib)
