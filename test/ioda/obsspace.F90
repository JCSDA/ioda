!
! (C) Copyright 2019 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
!
!> Test interface for C++ ObsSpace called from Fortran

#include <fckit/fctest.h>

module ObsSpaceTestFixture
  use, intrinsic :: iso_c_binding
  
  implicit none
  
  type(c_ptr) :: ptr = c_null_ptr
end module

TESTSUITE_WITH_FIXTURE(obsspace_f, ObsSpaceTestFixture)

TESTSUITE_INIT
  use fckit_module
  call fckit_main%init()
END_TESTSUITE_INIT

TESTSUITE_FINALIZE
END_TESTSUITE_FINALIZE

!> Test c_obsspace_construct
TEST(test_c_obsspace_construct)
  use fckit_configuration_module
  use fckit_pathname_module, only : fckit_pathname
  use fckit_module
  implicit none

  character(len=:), allocatable :: filename
  type(fckit_configuration) :: config

  call fckit_resource("-config", "", filename)
  config = fckit_YAMLConfiguration(fckit_pathname(filename))
  print *, config.has("test")
  print *, "i am writing in c_obsspace_construct test"
END_TEST

END_TESTSUITE
