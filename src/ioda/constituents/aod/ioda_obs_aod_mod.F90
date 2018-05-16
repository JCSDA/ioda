!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran module handling aod observation space

module ioda_obs_aod_mod

use kinds
use read_aod_diag, only: set_aoddiag,&
                         diag_header_fix_list_aod,&
                         diag_header_chan_list_aod,&
                         diag_data_name_list_aod,&
                         diag_data_chan_list_aod,&
                         diag_data_fix_list_aod

use read_aod_diag, only: set_netcdf_read_aod,&
                         read_all_aoddiag,&
                         read_aoddiag_header
use fckit_log_module, only : fckit_log

implicit none
private
character(len=*),parameter :: myname ="aodNode_mod"
integer, parameter :: max_string=800

public ioda_obs_aod
public ioda_obs_aod_setup, ioda_obs_aod_delete
public ioda_obs_aod_read, ioda_obs_aod_generate
public ioda_obs_aod_getlocs

! ------------------------------------------------------------------------------

!> Fortran derived type to hold observation locations
type :: ioda_obs_aod
  integer :: nobs
  integer :: nlocs
  type(diag_header_fix_list_aod )              ::  header_fix
  type(diag_header_chan_list_aod),allocatable  ::  header_chan(:)
  type(diag_data_name_list_aod)                ::  header_name
  TYPE(diag_data_fix_list_aod), allocatable    ::  datafix(:)
  TYPE(diag_data_chan_list_aod) ,ALLOCATABLE   ::  datachan(:,:)
end type ioda_obs_aod

! ------------------------------------------------------------------------------

contains

! ------------------------------------------------------------------------------

subroutine ioda_obs_aod_setup(self, nobs)
implicit none
type(ioda_obs_aod), intent(inout) :: self
integer, intent(in) :: nobs

call ioda_obs_aod_delete(self)

self%nobs = nobs

end subroutine ioda_obs_aod_setup

! ------------------------------------------------------------------------------

subroutine ioda_obs_aod_delete(self)
implicit none
type(ioda_obs_aod), intent(inout) :: self

self%nobs = 0
self%nlocs = 0
if (allocated(self%header_chan)) deallocate(self%header_chan)
if (allocated(self%datafix)) deallocate(self%datafix)
if (allocated(self%datachan)) deallocate(self%datachan)

end subroutine ioda_obs_aod_delete

! ------------------------------------------------------------------------------

subroutine ioda_obs_aod_generate(self, nobs, lat, lon1, lon2)
implicit none
type(ioda_obs_aod), intent(inout) :: self
integer, intent(in) :: nobs
real, intent(in) :: lat, lon1, lon2

integer :: i

call ioda_obs_aod_setup(self, nobs)

self%datafix(:)%Lat = lat
do i = 1, nobs
  self%datafix(i)%Lon = lon1 + (i-1)*(lon2-lon1)/(nobs-1)
enddo

print *, 'in random:', self%nobs, self%datafix%lon, self%datafix%lat

end subroutine ioda_obs_aod_generate

! ------------------------------------------------------------------------------

subroutine ioda_obs_aod_read(filename, self)
use nc_diag_read_mod, only: nc_diag_read_get_var, nc_diag_read_get_dim
use nc_diag_read_mod, only: nc_diag_read_init, nc_diag_read_close

use m_diag_raob, only: read_raob_diag_nc_header, read_raob_diag_nc_mass

implicit none
character(max_string), intent(in)   :: filename
type(ioda_obs_aod), intent(inout), target :: self

character(len=*),parameter :: myname_ =myname//"*aod_read"
integer :: ier
integer :: luin=0
logical :: lverbose  = .true.  ! control verbose

call ioda_obs_aod_delete(self)

call set_netcdf_read_aod(.true.)
CALL nc_diag_read_init(filename, luin)

call read_aoddiag_header(luin,self%header_fix,self%header_chan,self%header_name,ier,lverbose)

print*, myname_, ': Found this many channels: ', self%header_fix%nchan
print*, myname_, ': Observation type in file: ', self%header_fix%obstype
print*, myname_, ': Date of input file:       ', self%header_fix%idate


CALL read_all_aoddiag ( luin, self%header_fix,self%datafix, self%datachan, self%nlocs, ier)

self%nobs  = self%nlocs * self%header_fix%nchan

print *, myname_, ' Total number of observations in file: ', self%nobs
CALL nc_diag_read_close(filename=filename)

end subroutine ioda_obs_aod_read

! ------------------------------------------------------------------------------

subroutine ioda_obs_aod_getlocs(self, locs)
use ioda_locs_mod
implicit none
type(ioda_obs_aod), intent(in) :: self
type(ioda_locs), intent(inout) :: locs

CHARACTER(len=*),PARAMETER:: myname_=myname//"*aod_getlocs"
character(len=255) :: record
integer :: failed

call ioda_locs_setup(locs, self%nlocs)

failed=0
if(failed==0 .and. size(self%datafix(:)%Lat)==self%nlocs) then
  locs%lat(:) = self%datafix(:)%Lat
else
  failed=1
endif
if(failed==0 .and. size(self%datafix(:)%Lon)==self%nlocs) then
  locs%lon(:) = self%datafix(:)%Lon
else
  failed=2
endif
if(failed==0 .and. size(self%datafix(:)%obstime)==self%nlocs) then
  locs%time(:) = self%datafix(:)%obstime
else
  failed=3
endif

if(failed==0)then
  WRITE(record,*)myname_,': allocated/assinged obs-data'
  call fckit_log%info(record)
else
  WRITE(record,*)myname_,': failed allocation/assignment of obs-data, ier: ', failed
  call fckit_log%info(record)
  ! should exit in error here
endif

end subroutine ioda_obs_aod_getlocs

! ------------------------------------------------------------------------------

end module ioda_obs_aod_mod
