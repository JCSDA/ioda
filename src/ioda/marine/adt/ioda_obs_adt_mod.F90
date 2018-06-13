!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran module handling adt observation space

module ioda_obs_adt_mod

use kinds

implicit none
private
integer, parameter :: max_string=800

public ioda_obs_adt
public ioda_obs_adt_setup, ioda_obs_adt_delete
public ioda_obs_adt_read, ioda_obs_adt_generate
public ioda_obs_adt_getlocs

! ------------------------------------------------------------------------------

!> Fortran derived type to hold observation locations
type :: ioda_obs_adt
  integer :: nobs
  real(kind_real), allocatable, dimension(:) :: lat      !< latitude
  real(kind_real), allocatable, dimension(:) :: lon      !< longitude
  real(kind_real), allocatable, dimension(:) :: adt  !< adt 
  real(kind_real), allocatable, dimension(:) :: adt_err  !< adt 
end type ioda_obs_adt

! ------------------------------------------------------------------------------
contains

! ------------------------------------------------------------------------------

subroutine ioda_obs_adt_setup(self, nobs)
implicit none

type(ioda_obs_adt), intent(inout) :: self
integer, intent(in) :: nobs

call ioda_obs_adt_delete(self)

self%nobs = nobs
allocate(self%lat(nobs), self%lon(nobs))
allocate(self%adt(nobs),self%adt_err(nobs))
self%lat = 0.
self%lon = 0.
self%adt = 0.
self%adt_err = 0.

end subroutine ioda_obs_adt_setup

! ------------------------------------------------------------------------------

subroutine ioda_obs_adt_delete(self)
implicit none
type(ioda_obs_adt), intent(inout) :: self

self%nobs = 0
if (allocated(self%lat)) deallocate(self%lat)
if (allocated(self%lon)) deallocate(self%lon)
if (allocated(self%adt)) deallocate(self%adt)
if (allocated(self%adt_err)) deallocate(self%adt_err)

end subroutine ioda_obs_adt_delete

! ------------------------------------------------------------------------------

subroutine ioda_obs_adt_generate(self, nobs, lat, lon1, lon2)
implicit none
type(ioda_obs_adt), intent(inout) :: self
integer, intent(in) :: nobs
real, intent(in) :: lat, lon1, lon2

integer :: i

call ioda_obs_adt_setup(self, nobs)

self%adt(:) = 3.0
self%adt_err(:) = 0.1

self%lat(:)     = lat
do i = 1, nobs
  self%lon(i) = lon1 + (i-1)*(lon2-lon1)/(nobs-1)
enddo

end subroutine ioda_obs_adt_generate

! ------------------------------------------------------------------------------

subroutine ioda_obs_adt_read(filename, self)
use nc_diag_read_mod, only: nc_diag_read_get_var, nc_diag_read_get_dim
use nc_diag_read_mod, only: nc_diag_read_init, nc_diag_read_close
use ncd_kinds, only: i_short
implicit none
character(max_string), intent(in)   :: filename
type(ioda_obs_adt), intent(inout) :: self

integer :: iunit, nt, nr, nc, nobs, qcnobs
integer(i_short), allocatable, dimension(:)    :: ssh
real(kind_real), allocatable, dimension(:)    :: qc
integer, allocatable, dimension(:)    :: lon, lat, mdt

integer :: i, qci
real :: undef = -99999.9

call ioda_obs_adt_delete(self)

! open netcdf file and read dimensions
call nc_diag_read_init(filename, iunit)
nt = nc_diag_read_get_dim(iunit,'time')
nobs = nt
allocate(lon(nobs), lat(nobs), ssh(nobs), qc(nobs), mdt(nobs))
call nc_diag_read_get_var(iunit, "lat", lat)
call nc_diag_read_get_var(iunit, "lon", lon)
call nc_diag_read_get_var(iunit, "ssha", ssh)
call nc_diag_read_get_var(iunit, "mean_topography", mdt)
call nc_diag_read_close(filename)

qc = 1
where ( (lat.eq.0).or.(ssh.gt.9999).or.(mdt.gt.9999) )
   qc=0
end where

qcnobs = sum(qc)
self%nobs = qcnobs
print *,qcnobs, nobs

! allocate geovals structure
call ioda_obs_adt_setup(self, qcnobs)

qci = 0
do i = 1, nobs
   if ( qc(i).eq.1 ) then
      qci = qci + 1
      self%lat(qci) = lat(i)*1e-6      
      self%lon(qci) = lon(i)*1e-6
      if (self%lon(qci)>80.0) self%lon(qci) = self%lon(qci) - 360.0
      self%adt(qci) = ssh(i)*0.001 + mdt(i)*0.0001
      self%adt_err(qci) = 0.1
      write(701,*)self%lon(qci),self%lat(qci),self%adt(qci)
   end if
end do

!self%madt = 0.0
!self%qc = 1
deallocate(lon, lat, ssh, qc, mdt)

print *, 'in read: ', self%nobs, nobs

end subroutine ioda_obs_adt_read

! ------------------------------------------------------------------------------

!!$subroutine ioda_obs_adt_read(filename, self)
!!$use nc_diag_read_mod, only: nc_diag_read_get_var, nc_diag_read_get_dim
!!$use nc_diag_read_mod, only: nc_diag_read_init, nc_diag_read_close
!!$implicit none
!!$character(max_string), intent(in)   :: filename
!!$type(ioda_obs_adt), intent(inout) :: self
!!$
!!$integer :: iunit, nobs, i
!!$real, dimension(:), allocatable :: field
!!$real, dimension(:), allocatable :: field_int
!!$
!!$call ioda_obs_adt_delete(self)
!!$
!!$call nc_diag_read_init(filename, iunit)
!!$nobs = nc_diag_read_get_dim(iunit,'nobs')
!!$print *,'nobs:',nobs
!!$self%nobs = nobs
!!$
!!$allocate(field(nobs))
!!$allocate(field_int(nobs))
!!$allocate(self%lon(nobs), self%lat(nobs), self%adt(nobs))
!!$allocate(self%adt_err(nobs))
!!$
!!$call nc_diag_read_get_var(iunit, "lat", field_int)
!!$self%lat = field_int
!!$call nc_diag_read_get_var(iunit, "lon", field_int)
!!$self%lon = field_int
!!$call nc_diag_read_get_var(iunit, "adt_unfiltered", field)
!!$self%adt = field
!!$call nc_diag_read_close(filename)
!!$self%adt_err = 0.01
!!$
!!$! Open netcdf file and read dimensions
!!$call nc_diag_read_init(filename, iunit)
!!$nt = nc_diag_read_get_dim(iunit,'time')
!!$nobs = nt
!!$allocate(lon(nobs), lat(nobs), ssh(nobs), qc(nobs), mdt(nobs))
!!$call nc_diag_read_get_var(iunit, "lat", lat)
!!$call nc_diag_read_get_var(iunit, "lon", lon)
!!$call nc_diag_read_get_var(iunit, "ssha", ssh)
!!$call nc_diag_read_get_var(iunit, "mean_topography", mdt)
!!$call nc_diag_read_close(filename)
!!$
!!$
!!$do i=1,nobs
!!$   write(301,*)self%lon(i),self%lat(i),self%adt(i)
!!$!   print *,'obs is',i,self%lon(i),self%lat(i),self%adt(i)
!!$end do
!!$end subroutine ioda_obs_adt_read

! ------------------------------------------------------------------------------

subroutine ioda_obs_adt_getlocs(self, locs)
use ioda_locs_mod
implicit none
type(ioda_obs_adt), intent(in) :: self
type(ioda_locs), intent(inout) :: locs

call ioda_locs_setup(locs, self%nobs)
locs%lat = self%lat
locs%lon = self%lon
locs%time = 0.

end subroutine ioda_obs_adt_getlocs

! ------------------------------------------------------------------------------

end module ioda_obs_adt_mod
