# (C) Copyright 2021 UCAR.
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

include(Targets)

ecbuild_add_test ( TARGET     test_ioda-engines_sfuncs_concatstringvectors
                   SOURCES    test-concatstringvecs.cpp
                   LIBS       ioda_engines )

ecbuild_add_test ( TARGET     test_ioda-engines_sfuncs_convertv1pathtov2path
                   SOURCES    test-convertv1pathtov2path.cpp
                   LIBS       ioda_engines )

# Buggy. Disabled.
#add_executable(ioda-test-types ioda-test-types.cpp)
#addapp(ioda-test-types)
#target_link_libraries(ioda-test-types PUBLIC ioda_engines)

#if(BUILD_TESTING)
#    add_test(NAME ioda-test-types COMMAND ioda-test-types)
#endif()

