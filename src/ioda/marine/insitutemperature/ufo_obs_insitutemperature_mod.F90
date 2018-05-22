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
if (allocated(self%val)) deallocate(self%val)
if (allocated(self%err)) deallocate(self%err)

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
   print *,self%lon(i)
  !self%depth(i) = self%depth(i) + 2.0*mod(i,nlev)               ! put obs every 2m
  !self%val(i) = self%val(i) * exp( -1.0 * mod(i,nlev)/halflev ) ! Scale the temperature measurement by exp function in depth
enddo

end subroutine ioda_obs_insitutemperature_generate

! ------------------------------------------------------------------------------

subroutine ioda_obs_insitutemperature_read(filename, self)
use nc_diag_read_mod, only: nc_diag_read_get_var, nc_diag_read_get_dim
use nc_diag_read_mod, only: nc_diag_read_init, nc_diag_read_close
implicit none
character(max_string), intent(in)   :: filename
type(ioda_obs_insitutemperature), intent(inout) :: self

!type(ioda_obs_insitutemperature) :: self
integer :: iunit, nobs, nprf, i, j, k
integer, dimension(:), allocatable :: pidx
integer, dimension(:), allocatable :: plen
real, dimension(:), allocatable :: buf4
real, dimension(:), allocatable :: plon, plat, ptime

integer :: psum, nn
logical :: dodebug = .false.

call ioda_obs_insitutemperature_delete(self)
call nc_diag_read_init(filename, iunit)

nobs = nc_diag_read_get_dim(iunit,'obs')
self%nobs = nobs

nprf = nc_diag_read_get_dim(iunit,'prfs')
self%nprf = nprf

if (dodebug) then
  print *, "nobs = ", nobs
  print *, "nprf = ", nprf 
! stop 99
endif

! Check to make sure nobs > nprf (!STEVE: add this check)
if (nobs < nprf) then
  ! ERROR
endif

! Initialize self:
call ioda_obs_insitutemperature_setup(self, nobs)

!-----------------------------------------------------
! Read the observation data that is dimensioned 'nobs'
!-----------------------------------------------------
allocate(buf4(nobs))

! Read the profile's depths
call nc_diag_read_get_var(iunit, "obs_depth", buf4)
self%depth=buf4

! Read the profile's measurement values
call nc_diag_read_get_var(iunit, "obs_val", buf4)
self%val=buf4

! Read the observation error estimate for each ob/depth
!call nc_diag_read_get_var(iunit, "obs_err", buf4)
!self%err=buf4

! Read the observation qc flag for each ob/depth
!call nc_diag_read_get_var(iunit, "obs_qc", buf4)
!self%qcflag=buf4

!-----------------------------------------------------
! Read the observation data that is dimensioned 'nprf'
!-----------------------------------------------------
allocate(plon(nprf),plat(nprf),ptime(nprf))
allocate(pidx(nprf),plen(nprf))

! Read the profile's observation index
call nc_diag_read_get_var(iunit, "prf_obsidx", pidx)

! Read the profile's longitude / latitude / time coordinate
call nc_diag_read_get_var(iunit, "prf_lon", plon)
if (dodebug) print *, "plon = ", plon
call nc_diag_read_get_var(iunit, "prf_lat", plat)
if (dodebug) print *, "plat = ", plat
call nc_diag_read_get_var(iunit, "prf_hr", ptime)
if (dodebug) print *, "ptime = ", ptime

! Close the file
call nc_diag_read_close(filename)

!-----------------------------------------------------
! Process data
!-----------------------------------------------------

! Compute the profile length (until it can be read in)
do j=1,nprf-1
  plen(j) = pidx(j+1) - pidx(j)
  !if (dodebug) print *, "plen(",j,") = ", plen(j)
enddo
j = nprf
psum = sum(plen(1:nprf-1))
plen(j) = nobs - psum
if (dodebug) print *, "plen(",j,") = ", plen(j)

! Expand the observation's profile information
i = 0
do j=1,nprf
  do k=1,plen(j)
    i=i+1
    self%idx(i) = pidx(j)
    self%lon(i) = plon(j)
    self%lat(i) = plat(j)
    self%time(i) = ptime(j)
  enddo
enddo

!STEVE: hard-coded obs error
!       replace with geographically-dependent time-dependent approach
!self%err = field
self%err = 1.0
self%qcflag = 1

! For validation, write out the state info

!!$if (dodebug) then
  do i=1,nobs
     !print *, self%idx(i),self%lon(i),self%lat(i),self%depth(i),self%time(i),self%val(i),self%err(i)
  enddo
!!$endif

print *, "nobs = ", nobs
print *, "nprf = ", nprf 

!!$nn=10
!!$call ioda_obs_insitutemperature_setup(self, nn)

!!$self%nobs=nn
!!$self%lat=self_tmp%lat(1:nn)
!!$self%lon=self_tmp%lon(1:nn)
!!$self%depth=self_tmp%depth(1:nn)
!!$self%time=self_tmp%time(1:nn)
!!$self%val=self_tmp%val(1:nn)
!!$self%err=self_tmp%err(1:nn)
!!$self%qcflag=self_tmp%qcflag(1:nn)
!!$
!!$call ioda_obs_insitutemperature_delete(self_tmp)

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

end module ioda_obs_insitutemperature_mod
