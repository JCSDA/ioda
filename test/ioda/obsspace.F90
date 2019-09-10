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
  use liboops_mod

  call liboops_initialise()
  call fckit_main%init()
END_TESTSUITE_INIT

TESTSUITE_FINALIZE
  use fckit_module
  use liboops_mod

  call fckit_main%final()
  call liboops_finalise()
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

  type(c_ptr), allocatable, dimension(:) :: obsspace
  integer :: nlocs, nlocs_ref
  integer :: nvars, nvars_ref
  integer :: iobstype

  call fckit_resource("-config", "", filename)
  config = fckit_YAMLConfiguration(fckit_pathname("testinput/iodatest_obsspace_fortran.yml"))
!filename))
  call config.get_or_die("window_begin", winbgnstr)
  call config.get_or_die("window_end", winendstr)
  call datetime_create(winbgnstr, winbgn)
  call datetime_create(winendstr, winend)
  call config.get_or_die("Observations.ObsTypes", obsconfigs)
  allocate(obsspace(size(obsconfigs)))
  do iobstype = 1, size(obsconfigs)
    call obsconfigs(iobstype).get_or_die("ObsSpace", obsconfig)
    obsspace(iobstype) = obsspace_construct(obsconfig, winbgn, winend)
    nlocs = obsspace_get_nlocs(obsspace(iobstype))
    nvars = obsspace_get_nvars(obsspace(iobstype))
    call obsconfig.get_or_die("TestData.nlocs", nlocs_ref)
    call obsconfig.get_or_die("TestData.nvars", nvars_ref)
    CHECK(nlocs == nlocs_ref)
    CHECK(nvars == nvars_ref)
  enddo
  do iobstype = 1, size(obsspace)
    call obsspace_destruct(obsspace(iobstype))
  enddo

END_TEST

END_TESTSUITE
