!
! (C) Copyright 2019 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
!
!> Test interface for C++ ObsSpace called from Fortran

#include <fckit/fctest.h>

TESTSUITE(obsspace_fortran)

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

!> Test obsspace_construct
TEST(test_obsspace_construct)
  use fckit_configuration_module
  use fckit_pathname_module, only : fckit_pathname
  use fckit_module
  use datetime_mod
  use obsspace_mod
  use, intrinsic :: iso_c_binding
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
  character(len=100) :: obsname
  character(kind=c_char,len=:), allocatable :: obsname_ref

  !> initialize winbgn, winend, get config
  call fckit_resource("--config", "", filename)
  config = fckit_YAMLConfiguration(fckit_pathname(filename))
  call config%get_or_die("window_begin", winbgnstr)
  call config%get_or_die("window_end", winendstr)
  call datetime_create(winbgnstr, winbgn)
  call datetime_create(winendstr, winend)
  !> allocate all ObsSpaces
  call config%get_or_die("Observations.ObsTypes", obsconfigs)
  allocate(obsspace(size(obsconfigs)))
  do iobstype = 1, size(obsconfigs)
    call obsconfigs(iobstype)%get_or_die("ObsSpace", obsconfig)
    !> construct obsspace
    obsspace(iobstype) = obsspace_construct(obsconfig, winbgn, winend)
    call obsspace_obsname(obsspace(iobstype), obsname)
    !> test if obsname is the same as reference
    call obsconfig%get_or_die("name", obsname_ref)
    CHECK_EQUAL(obsname, obsname_ref)
    !> test if nlocs and nvars are the same as reference
    nlocs = obsspace_get_nlocs(obsspace(iobstype))
    nvars = obsspace_get_nvars(obsspace(iobstype))
    call obsconfig%get_or_die("TestData.nlocs", nlocs_ref)
    call obsconfig%get_or_die("TestData.nvars", nvars_ref)
    CHECK_EQUAL(nlocs, nlocs_ref)
    CHECK_EQUAL(nvars,  nvars_ref)
  enddo
  !> destruct all obsspaces
  do iobstype = 1, size(obsspace)
    call obsspace_destruct(obsspace(iobstype))
  enddo
  deallocate(obsspace, obsname_ref)

END_TEST

END_TESTSUITE
