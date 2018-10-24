! (C) Copyright 2018 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!> Fortran module to handle ice concentration observations

module ioda_obs_seaicefrac_mod_c

use iso_c_binding
use string_f_c_mod
use config_mod
use datetime_mod
use duration_mod
use ioda_locs_mod
use ioda_locs_mod_c, only : ioda_locs_registry
use ioda_obs_vectors
use ioda_obs_seaicefrac_mod
use fckit_log_module, only : fckit_log
use kinds

implicit none
private

public :: ioda_obs_seaicefrac_registry

! ------------------------------------------------------------------------------
integer, parameter :: max_string=800
! ------------------------------------------------------------------------------

#define LISTED_TYPE ioda_obs_seaicefrac

!> Linked list interface - defines registry_t type
#include "../../linkedList_i.f"

!> Global registry
type(registry_t) :: ioda_obs_seaicefrac_registry

! ------------------------------------------------------------------------------
contains
! ------------------------------------------------------------------------------
!> Linked list implementation
#include "../../linkedList_c.f"

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_seaice_setup_c(c_key_self, c_conf) bind(c,name='ioda_obsdb_seaice_setup_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(c_ptr), intent(in)       :: c_conf !< configuration

type(ioda_obs_seaicefrac), pointer :: self
character(len=max_string)  :: fin
character(len=max_string)  :: MyObsType
character(len=255) :: record

if (config_element_exists(c_conf,"ObsData.ObsDataIn")) then
  fin  = config_get_string(c_conf,max_string,"ObsData.ObsDataIn.obsfile")
else
  fin  = ""
endif
MyObsType = trim(config_get_string(c_conf,max_string,"ObsType"))
write(record,*) 'ioda_obsdb_seaice_setup_c: ', trim(MyObsType), ' file in =',trim(fin)
call fckit_log%info(record)

call ioda_obs_seaicefrac_registry%init()
call ioda_obs_seaicefrac_registry%add(c_key_self)
call ioda_obs_seaicefrac_registry%get(c_key_self, self)
if (trim(fin) /= "") then
  call ioda_obs_seaicefrac_read(fin, self)
endif

end subroutine ioda_obsdb_seaice_setup_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_seaice_getlocations_c(c_key_self, c_t1, c_t2, c_key_locs) bind(c,name='ioda_obsdb_seaice_getlocations_f90')
implicit none
integer(c_int), intent(in)    :: c_key_self
type(c_ptr), intent(in)       :: c_t1, c_t2
integer(c_int), intent(inout) :: c_key_locs

type(ioda_obs_seaicefrac), pointer :: self
type(datetime) :: t1, t2
type(ioda_locs), pointer :: locs

call ioda_obs_seaicefrac_registry%get(c_key_self, self)
call c_f_datetime(c_t1, t1)
call c_f_datetime(c_t2, t2)

call ioda_locs_registry%init()
call ioda_locs_registry%add(c_key_locs)
call ioda_locs_registry%get(c_key_locs,locs)

call ioda_obs_seaicefrac_getlocs(self, locs)

end subroutine ioda_obsdb_seaice_getlocations_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_seaice_generate_c(c_key_self, c_conf, c_t1, c_t2) bind(c,name='ioda_obsdb_seaice_generate_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(c_ptr), intent(in)       :: c_conf !< configuration
type(c_ptr), intent(in)       :: c_t1, c_t2

type(ioda_obs_seaicefrac), pointer :: self
type(datetime) :: t1, t2
integer :: nobs
real :: lat, lon1, lon2

call ioda_obs_seaicefrac_registry%get(c_key_self, self)
call c_f_datetime(c_t1, t1)
call c_f_datetime(c_t2, t2)

nobs = config_get_int(c_conf, "nobs")
lat  = config_get_real(c_conf, "lat")
lon1 = config_get_real(c_conf, "lon1")
lon2 = config_get_real(c_conf, "lon2")

call ioda_obs_seaicefrac_generate(self, nobs, lat, lon1, lon2)

end subroutine ioda_obsdb_seaice_generate_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_seaice_nobs_c(c_key_self, kobs) bind(c,name='ioda_obsdb_seaice_nobs_f90')
implicit none
integer(c_int), intent(in) :: c_key_self
integer(c_int), intent(inout) :: kobs
type(ioda_obs_seaicefrac), pointer :: self

call ioda_obs_seaicefrac_registry%get(c_key_self, self)
kobs = self%nobs

end subroutine ioda_obsdb_seaice_nobs_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_seaice_delete_c(c_key_self) bind(c,name='ioda_obsdb_seaice_delete_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(ioda_obs_seaicefrac), pointer :: self

call ioda_obs_seaicefrac_registry%get(c_key_self, self)
call ioda_obs_seaicefrac_delete(self)
call ioda_obs_seaicefrac_registry%remove(c_key_self)

end subroutine ioda_obsdb_seaice_delete_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_seaice_get_c(c_key_self, lcol, c_col, c_key_ovec) bind(c,name='ioda_obsdb_seaice_get_f90')
use  ioda_obs_seaicefrac_mod
implicit none
integer(c_int), intent(in) :: c_key_self
integer(c_int), intent(in) :: lcol
character(kind=c_char,len=1), intent(in) :: c_col(lcol+1)
integer(c_int), intent(in) :: c_key_ovec

type(ioda_obs_seaicefrac), pointer :: self
type(obs_vector), pointer :: ovec
character(len=lcol) :: col

print *,'.................................in ioda_obsdbsic_get'

call ioda_obs_seaicefrac_registry%get(c_key_self, self)
call ioda_obs_vect_registry%get(c_key_ovec,ovec)
!call c_f_string(c_req, req)
!call c_f_string(c_col, col)

!call obs_get(self, trim(req), trim(col), ovec)


ovec%nobs = self%nobs
if (c_col(5)//c_col(6)=='rr') then
   ovec%values = 0.1 !self%icefrac_err
!   print *, self%icefrac_err
else
   ovec%values = self%icefrac
end if

print *,'................................. out of ioda_obsdbsic_get'

end subroutine ioda_obsdb_seaice_get_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_seaice_getvar_c(c_key_self, lcol, c_col, vdata, vsize) bind(c,name='ioda_obsdb_seaice_getvar_f90')  
implicit none
integer(c_int), intent(in) :: c_key_self
integer(c_int), intent(in) :: lcol
character(kind=c_char,len=1), intent(in) :: c_col(lcol+1)
real(kind=c_double), intent(out) :: vdata(vsize)
integer(c_int), value, intent(in) :: vsize

type(ioda_obs_seaicefrac), pointer :: self

character(len=lcol) :: vname
integer :: i

! Copy C character array to Fortran string
do i = 1, lcol
  vname(i:i) = c_col(i)
enddo

call ioda_obs_seaicefrac_registry%get(c_key_self, self)
call ioda_obs_seaicefrac_copy_var(self, vname, vdata, vsize)

end subroutine ioda_obsdb_seaice_getvar_c


end module ioda_obs_seaicefrac_mod_c
