# (C) Copyright 2022 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

if (odc_FOUND AND oops_FOUND AND eckit_FOUND)
	add_subdirectory(odc_converter)
endif()

add_subdirectory(upgrade)
