! (C) Copyright 2018 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!> Fortran example module for for functions on the interface between C++ and Fortran
!  to handle observation space

! TODO: replace example with your_observation_space_name through the file

module ioda_obs_example_mod_c

use iso_c_binding
use string_f_c_mod
use config_mod
use datetime_mod
use duration_mod
!use ioda_locs_mod
!use ioda_locs_mod_c, only : ioda_locs_registry
use ioda_obs_vectors
use ioda_obs_example_mod
use fckit_log_module, only : fckit_log
use kinds

implicit none
private

public :: ioda_obs_example_registry

! ------------------------------------------------------------------------------
integer, parameter :: max_string=800
! ------------------------------------------------------------------------------

#define LISTED_TYPE ioda_obs_example

!> Linked list interface - defines registry_t type
#include "../../linkedList_i.f"

!> Global registry
type(registry_t) :: ioda_obs_example_registry

! ------------------------------------------------------------------------------
contains
! ------------------------------------------------------------------------------
!> Linked list implementation
#include "../../linkedList_c.f"

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_example_setup_c(c_key_self, c_conf) bind(c,name='ioda_obsdb_example_setup_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(c_ptr), intent(in)       :: c_conf !< configuration

type(ioda_obs_example), pointer :: self
character(len=max_string)  :: fin
character(len=max_string)  :: MyObsType
character(len=255) :: record

if (config_element_exists(c_conf,"ObsData.ObsDataIn")) then
  fin  = config_get_string(c_conf,max_string,"ObsData.ObsDataIn.obsfile")
else
  fin  = ""
endif
call fckit_log%info(record)

call ioda_obs_example_registry%init()
call ioda_obs_example_registry%add(c_key_self)
call ioda_obs_example_registry%get(c_key_self, self)
if (trim(fin) /= "") then
! TODO: replace with the call to your Fortran routine for reading observations
!       (defined in ioda_obs_<your_obs_space_name>_mod.F90)
  call ioda_obs_example_read(fin, self)
endif

end subroutine ioda_obsdb_example_setup_c

! ------------------------------------------------------------------------------
!
!subroutine ioda_obsdb_example_getlocations_c(c_key_self, c_t1, c_t2, c_key_locs) bind(c,name='ioda_obsdb_example_getlocations_f90')
!implicit none
!integer(c_int), intent(in)    :: c_key_self
!type(c_ptr), intent(in)       :: c_t1, c_t2
!integer(c_int), intent(inout) :: c_key_locs
!
!type(ioda_obs_example), pointer :: self
!type(datetime) :: t1, t2
!type(ioda_locs), pointer :: locs
!
!call ioda_obs_example_registry%get(c_key_self, self)
!call c_f_datetime(c_t1, t1)
!call c_f_datetime(c_t2, t2)
!
!call ioda_locs_registry%init()
!call ioda_locs_registry%add(c_key_locs)
!call ioda_locs_registry%get(c_key_locs,locs)
!
!! TODO: replace with the call to your Fortran routine for getting locations of obs
!!       (defined in ioda_obs_<your_obs_space_name>_mod.F90)
!call ioda_obs_example_getlocs(self, locs)
!
!end subroutine ioda_obsdb_example_getlocations_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_example_generate_c(c_key_self, c_conf, c_t1, c_t2) bind(c,name='ioda_obsdb_example_generate_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(c_ptr), intent(in)       :: c_conf !< configuration
type(c_ptr), intent(in)       :: c_t1, c_t2

type(ioda_obs_example), pointer :: self
type(datetime) :: t1, t2
integer :: nobs

call ioda_obs_example_registry%get(c_key_self, self)
call c_f_datetime(c_t1, t1)
call c_f_datetime(c_t2, t2)

nobs = config_get_int(c_conf, "nobs")

! TODO: replace with the call to your Fortran routine for generating random observations
!       (defined in ioda_obs_<your_obs_space_name>_mod.F90)
call ioda_obs_example_generate(self, nobs)

end subroutine ioda_obsdb_example_generate_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_example_nobs_c(c_key_self, kobs) bind(c,name='ioda_obsdb_example_nobs_f90')
implicit none
integer(c_int), intent(in) :: c_key_self
integer(c_int), intent(inout) :: kobs
type(ioda_obs_example), pointer :: self

call ioda_obs_example_registry%get(c_key_self, self)

!TODO: call your function to inquire nobs from obsspace

end subroutine ioda_obsdb_example_nobs_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_example_delete_c(c_key_self) bind(c,name='ioda_obsdb_example_delete_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(ioda_obs_example), pointer :: self

call ioda_obs_example_registry%get(c_key_self, self)

! TODO: replace with the call to your Fortran routine to destruct the obsspace
!       (defined in ioda_obs_<your_obs_space_name>_mod.F90)
call ioda_obs_example_delete(self)

call ioda_obs_example_registry%remove(c_key_self)

end subroutine ioda_obsdb_example_delete_c

! ------------------------------------------------------------------------------

end module ioda_obs_example_mod_c
