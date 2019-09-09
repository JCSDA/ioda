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
  use fckit_module
  call fckit_main%final()
END_TESTSUITE_FINALIZE

!> Test c_obsspace_construct
TEST(test_c_obsspace_construct)
  use fckit_configuration_module
  use fckit_pathname_module, only : fckit_pathname
  use fckit_module
  use datetime_mod
  use obsspace_mod
  implicit none
  character(len=:), allocatable :: filename
  type(fckit_configuration) :: config
  type(fckit_configuration), allocatable :: obsconfigs(:)
  type(fckit_configuration) :: obsconfig
  character(kind=c_char,len=:), allocatable :: winbgnstr
  character(kind=c_char,len=:), allocatable :: winendstr
  type(datetime) :: winbgn, winend
  type(c_ptr) :: winbgn, winend
  type(c_ptr) :: obsspace
  integer     :: nlocs

  call fckit_resource("-config", "", filename)
  config = fckit_YAMLConfiguration(fckit_pathname("testinput/iodatest_obsspace_fortran.yml"))
!filename))
  call config.get_or_die("window_begin", winbgnstr)
  call config.get_or_die("window_end", winendstr)
  call datetime_create(winbgnstr, winbgn)
  call datetime_create(winendstr, winend)
  print *, "window: ", winbgnstr, " - ", winendstr
  call config.get_or_die("Observations.ObsTypes", obsconfigs)
  print *, "there are ", size(obsconfigs), " obstypes"
  call obsconfigs(1).get_or_die("ObsSpace", obsconfig)
  obsspace =  obsspace_construct(obsconfig, winbgn, winend)
  nlocs = obsspace_get_nlocs(obsspace)
  print *, "nlocs: ", nlocs
  call obsspace_destruct(obsspace)
  print *, "testing if I can still print something"
!  obsspace = c_null_ptr

END_TEST

END_TESTSUITE
