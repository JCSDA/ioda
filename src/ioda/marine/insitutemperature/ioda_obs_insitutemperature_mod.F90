!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran module handling sea temperature profile observation space

module ioda_obs_insitutemperature_mod

use kinds

implicit none
private
integer, parameter :: max_string=800

public ioda_obs_insitutemperature
public ioda_obs_insitutemperature_setup, ioda_obs_insitutemperature_delete
public ioda_obs_insitutemperature_read, ioda_obs_insitutemperature_generate
public ioda_obs_insitutemperature_getlocs
public ioda_obs_insitutemperature_copy_var

! ------------------------------------------------------------------------------

!> Fortran derived type to hold observation locations
type :: ioda_obs_insitutemperature
  integer :: nobs
  integer :: nprf
  integer, allocatable, dimension(:) :: idx                !< profile index
  real(kind_real), allocatable, dimension(:) :: lat        !< latitude
  real(kind_real), allocatable, dimension(:) :: lon        !< longitude
  real(kind_real), allocatable, dimension(:) :: depth      !< depth (m)
  real(kind_real), allocatable, dimension(:) :: time       !< float hour
  real(kind_real), allocatable, dimension(:) :: val        !< temperature value
  real(kind_real), allocatable, dimension(:) :: err        !< estimated observation error
  real(kind_real), allocatable, dimension(:) :: qcflag     !< quality control flag
end type ioda_obs_insitutemperature

! ------------------------------------------------------------------------------
contains

! ------------------------------------------------------------------------------

subroutine ioda_obs_insitutemperature_setup(self, nobs)
implicit none

type(ioda_obs_insitutemperature), intent(inout) :: self
integer, intent(in) :: nobs

call ioda_obs_insitutemperature_delete(self)

self%nobs = nobs
allocate(self%lon(nobs),self%lat(nobs),self%depth(nobs),self%time(nobs))
allocate(self%val(nobs),self%err(nobs))
allocate(self%idx(nobs),self%qcflag(nobs))
self%lat = 0.
self%lon = 0.
self%depth = 0.
self%val = 0.
self%err = 0.
self%idx = 0.
self%time = 0.
self%qcflag = 0.

end subroutine ioda_obs_insitutemperature_setup

! ------------------------------------------------------------------------------

subroutine ioda_obs_insitutemperature_delete(self)
implicit none
type(ioda_obs_insitutemperature), intent(inout) :: self

self%nobs = 0

if (allocated(self%lat)) deallocate(self%lat)
if (allocated(self%lon)) deallocate(self%lon)
if (allocated(self%depth)) deallocate(self%depth)
if (allocated(self%time)) deallocate(self%time)
if (allocated(self%val)) deallocate(self%val)
if (allocated(self%err)) deallocate(self%err)
if (allocated(self%idx)) deallocate(self%idx)
if (allocated(self%qcflag)) deallocate(self%qcflag)

end subroutine ioda_obs_insitutemperature_delete

! ------------------------------------------------------------------------------

subroutine ioda_obs_insitutemperature_generate(self, nobs, lat, lon1, lon2)
implicit none
type(ioda_obs_insitutemperature), intent(inout) :: self
integer, intent(in) :: nobs
real, intent(in) :: lat, lon1, lon2
integer :: nlev, nprf
integer :: halflev
integer :: i


! A set of sample observations to be used for testing

!STEVE: update so this isn't hard-coded

call ioda_obs_insitutemperature_setup(self, nobs)

self%val(:) = 27.5  ! (degC)
self%err(:) = 1.0   ! (degC)
self%depth(:) = 10. ! (m)
halflev = 10

self%lat(:) = lat
do i = 1, nobs
   self%lon(i) = lon1 + (i-1)*(lon2-lon1)/(nobs-1)               ! put obs along a latitude circle
  !self%depth(i) = self%depth(i) + 2.0*mod(i,nlev)               ! put obs every 2m
  !self%val(i) = self%val(i) * exp( -1.0 * mod(i,nlev)/halflev ) ! Scale the temperature measurement by exp function in depth
enddo

end subroutine ioda_obs_insitutemperature_generate

! ------------------------------------------------------------------------------

subroutine ioda_obs_insitutemperature_read(filename, self)
use m_diag_marine_conv
implicit none
character(max_string), intent(in)   :: filename
type(ioda_obs_insitutemperature), intent(inout) :: self

type(diag_marine_conv_tracer),pointer :: tao_tracer(:)
integer :: ierr, nobs, iobs

call ioda_obs_insitutemperature_delete(self)

call read_marine_conv_diag_nc_tracer(filename, tao_tracer, ierr)

nobs=size(tao_tracer,1)

call ioda_obs_insitutemperature_setup(self, nobs)
self%nobs   = nobs
self%idx    = 1
self%depth  = tao_tracer(:)%Station_Depth
self%val    = tao_tracer(:)%Observation
self%lon    = tao_tracer(:)%Longitude
self%lat    = tao_tracer(:)%Latitude
self%time   = tao_tracer(:)%Time
self%err    = 0.01
self%qcflag = 1

do iobs = 1, nobs
   write(401,*)self%lon(iobs),self%lat(iobs),self%val(iobs),self%depth(iobs)
end do

end subroutine ioda_obs_insitutemperature_read

! ------------------------------------------------------------------------------

subroutine ioda_obs_insitutemperature_getlocs(self, locs)
use ioda_locs_mod
implicit none
type(ioda_obs_insitutemperature), intent(in) :: self
type(ioda_locs), intent(inout) :: locs

call ioda_locs_setup(locs, self%nobs)
locs%lat = self%lat
locs%lon = self%lon
locs%time = self%time

end subroutine ioda_obs_insitutemperature_getlocs

! ------------------------------------------------------------------------------

subroutine ioda_obs_insitutemperature_copy_var(self, vname, vdata, vsize)
  implicit none

  type(ioda_obs_insitutemperature), intent(in) :: self
  character(len=*), intent(in)                 :: vname
  real(kind_real), intent(out)                 :: vdata(vsize)
  integer, intent(in)                          :: vsize

  character(max_string) :: err_msg

  if (trim(vname) .eq. "latitude") then
    vdata = self%lat
  elseif (trim(vname) .eq. "longitude") then
    vdata = self%lon
  elseif (trim(vname) .eq. "depth") then
    vdata = self%depth
  elseif (trim(vname) .eq. "time") then
    vdata = self%time
  elseif (trim(vname) .eq. "in_situ_temperature") then
    vdata = self%val
  elseif (trim(vname) .eq. "in_situ_temperature_err") then
    vdata = self%err
  elseif (trim(vname) .eq. "qcflag") then
    vdata = self%qcflag
  else
    write(err_msg,*) 'ioda_obs_insitutemperature_copy_var: Unrecognized variable name: ', trim(vname)
    call abor1_ftn(trim(err_msg))
  endif

end subroutine ioda_obs_insitutemperature_copy_var


end module ioda_obs_insitutemperature_mod
