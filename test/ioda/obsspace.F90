!
! (C) Copyright 2019-2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
!
!> Test interface for C++ ObsSpace called from Fortran

#include <fckit/fctest.h>

!> \cond
TESTSUITE(obsspace_fortran)

TESTSUITE_INIT
  use fckit_module
  use liboops_mod

  call fckit_main%init()
  call liboops_initialise()
END_TESTSUITE_INIT

TESTSUITE_FINALIZE
  use fckit_module
  use liboops_mod

  call liboops_finalise()
  call fckit_main%final()
END_TESTSUITE_FINALIZE

!> Test obsspace_construct
TEST(test_obsspace_construct)
  use fckit_configuration_module
  use fckit_pathname_module, only : fckit_pathname
  use fckit_module
  use datetime_mod
  use obsspace_mod
  use oops_variables_mod
  use, intrinsic :: iso_c_binding
  implicit none

  character(len=:), allocatable :: filename
  type(fckit_configuration) :: config
  type(fckit_configuration), allocatable :: obsconfigs(:)
  type(fckit_configuration) :: obsconfig
  type(fckit_configuration) :: testconfig
  type(fckit_configuration) :: timewinconfig

  type(c_ptr), allocatable, dimension(:) :: obsspace
  integer :: nlocs, nlocs_ref, location_id
  integer :: nvars_ref
  integer :: iobstype
  character(len=100) :: obsname
  character(kind=c_char,len=:), allocatable :: obsname_ref
  type(oops_variables) :: vars

  !> initialize winbgn, winend, get config
  call fckit_resource("--config", "", filename)
  config = fckit_YAMLConfiguration(fckit_pathname(filename))

  call config%get_or_die("time window", timewinconfig)

  !> allocate all ObsSpaces
  call config%get_or_die("observations", obsconfigs)
  allocate(obsspace(size(obsconfigs)))
  do iobstype = 1, size(obsconfigs)
    !> get the obs space and test data config pair for this ObsSpace
    call obsconfigs(iobstype)%get_or_die("obs space", obsconfig)
    call obsconfigs(iobstype)%get_or_die("test data", testconfig)

    !> construct obsspace
    obsspace(iobstype) = obsspace_construct(obsconfig, timewinconfig)
    call obsspace_obsname(obsspace(iobstype), obsname)

    !> test if obsname is the same as reference
    call obsconfig%get_or_die("name", obsname_ref)
    CHECK_EQUAL(obsname, obsname_ref)

    !> test if nlocs and nlocs are the same as reference
    location_id = obsspace_get_dim_id(obsspace(iobstype), "Location")
    nlocs = obsspace_get_dim_size(obsspace(iobstype), location_id)
    call testconfig%get_or_die("nlocs", nlocs_ref)
    CHECK_EQUAL(nlocs, nlocs_ref)

    !> test if obsvariables nvars is the same
    vars = obsspace_obsvariables(obsspace(iobstype))
    call testconfig%get_or_die("nvars obsvars", nvars_ref)
    CHECK_EQUAL(vars%nvars(), nvars_ref)
  enddo

  !> destruct all obsspaces
  do iobstype = 1, size(obsspace)
    call obsspace_destruct(obsspace(iobstype))
  enddo
  deallocate(obsspace, obsname_ref)

END_TEST

!> Test obsspace_get_db and obsspace_put_db
TEST(test_obsspace_get_db_put_db)
  use fckit_configuration_module
  use fckit_pathname_module, only : fckit_pathname
  use fckit_module
  use datetime_mod
  use obsspace_mod
  use oops_variables_mod
  use, intrinsic :: iso_c_binding
  implicit none

  character(len=:), allocatable :: filename
  type(fckit_configuration) :: config
  type(fckit_configuration), allocatable :: obsconfigs(:)
  type(fckit_configuration) :: obsconfig
  type(fckit_configuration) :: timewinconfig

  character(kind=c_char,len=:), allocatable :: winbgnstr
  character(kind=c_char,len=:), allocatable :: winendstr
  character(len=20) :: winbgnreadstr, winendreadstr
  type(datetime) :: winbgnread, winendread

  type(c_ptr), allocatable, dimension(:) :: obsspace
  integer :: nlocs, location_id
  integer :: iobstype, iloc

  integer(c_int32_t), allocatable :: input_int32_var(:),  output_int32_var(:)
  integer(c_int64_t), allocatable :: input_int64_var(:),  output_int64_var(:)
  real(c_float),      allocatable :: input_float_var(:),  output_float_var(:)
  real(c_double),     allocatable :: input_double_var(:), output_double_var(:)
  logical(c_bool),    allocatable :: input_bool_var(:),   output_bool_var(:)

  !> get config
  call fckit_resource("--config", "", filename)
  config = fckit_YAMLConfiguration(fckit_pathname(filename))

  call config%get_or_die("time window", timewinconfig)

  call timewinconfig%get_or_die("begin", winbgnstr)
  call timewinconfig%get_or_die("end", winendstr)

  !> allocate all ObsSpaces
  call config%get_or_die("observations", obsconfigs)
  allocate(obsspace(size(obsconfigs)))
  do iobstype = 1, size(obsconfigs)
    !> get the obs space config for this ObsSpace
    call obsconfigs(iobstype)%get_or_die("obs space", obsconfig)

    !> construct obsspace
    obsspace(iobstype) = obsspace_construct(obsconfig, timewinconfig)

    location_id = obsspace_get_dim_id(obsspace(iobstype), "Location")
    nlocs = obsspace_get_dim_size(obsspace(iobstype), location_id)

    !> test putting and getting a 32-bit int variable
    allocate(input_int32_var(nlocs), output_int32_var(nlocs))
    input_int32_var(1:nlocs:2) = 1
    input_int32_var(2:nlocs:2) = 2
    call obsspace_put_db(obsspace(iobstype), "MyGroup", "int32", input_int32_var)
    call obsspace_get_db(obsspace(iobstype), "MyGroup", "int32", output_int32_var)
    CHECK_EQUAL(input_int32_var(:), output_int32_var(:))
    deallocate(input_int32_var, output_int32_var)

    !> test putting and getting a 64-bit int variable
    allocate(input_int64_var(nlocs), output_int64_var(nlocs))
    input_int64_var(1:nlocs:2) = 3
    input_int64_var(2:nlocs:2) = 4
    call obsspace_put_db(obsspace(iobstype), "MyGroup", "int64", input_int64_var)
    call obsspace_get_db(obsspace(iobstype), "MyGroup", "int64", output_int64_var)
    CHECK_EQUAL(input_int64_var(:), output_int64_var(:))
    deallocate(input_int64_var, output_int64_var)

    !> test putting and getting a single-precision real variable
    allocate(input_float_var(nlocs), output_float_var(nlocs))
    input_float_var(1:nlocs:2) = 5.5
    input_float_var(2:nlocs:2) = 6.5
    call obsspace_put_db(obsspace(iobstype), "MyGroup", "float", input_float_var)
    call obsspace_get_db(obsspace(iobstype), "MyGroup", "float", output_float_var)
    CHECK_EQUAL(input_float_var(:), output_float_var(:))
    deallocate(input_float_var, output_float_var)

    !> test putting and getting a single-precision real variable
    allocate(input_double_var(nlocs), output_double_var(nlocs))
    input_double_var(1:nlocs:2) = 7.5
    input_double_var(2:nlocs:2) = 8.5
    call obsspace_put_db(obsspace(iobstype), "MyGroup", "double", input_double_var)
    call obsspace_get_db(obsspace(iobstype), "MyGroup", "double", output_double_var)
    CHECK_EQUAL(input_double_var(:), output_double_var(:))
    deallocate(input_double_var, output_double_var)

    !> test putting and getting a Boolean variable
    allocate(input_bool_var(nlocs), output_bool_var(nlocs))
    input_bool_var(1:nlocs:2) = .true.
    input_bool_var(2:nlocs:2) = .false.
    call obsspace_put_db(obsspace(iobstype), "MyGroup", "bool", input_bool_var)
    call obsspace_get_db(obsspace(iobstype), "MyGroup", "bool", output_bool_var)
    do iloc = 1, nlocs
      CHECK(input_bool_var(iloc) .eqv. output_bool_var(iloc))
    enddo
    deallocate(input_bool_var, output_bool_var)

    !> test get method for datetime produces equivalent ouput to input string
    call obsspace_get_window(obsspace(iobstype), winbgnread, winendread)
    call datetime_to_string(winbgnread, winbgnreadstr)
    CHECK_EQUAL(winbgnstr, winbgnreadstr)
    call datetime_to_string(winendread, winendreadstr)
    CHECK_EQUAL(winendstr, winendreadstr)
    call datetime_delete(winbgnread)
    call datetime_delete(winendread)

  enddo

  !> destruct all obsspaces
  do iobstype = 1, size(obsspace)
    call obsspace_destruct(obsspace(iobstype))
  enddo
  deallocate(obsspace)

END_TEST

END_TESTSUITE
!> \endcond
