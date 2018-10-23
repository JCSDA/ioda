!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran module handling observation locations

module ioda_obs_seasurfacetemp_mod

use kinds

implicit none
private
integer, parameter :: max_string=800

public ioda_obs_seasurfacetemp
public ioda_obs_seasurfacetemp_setup, ioda_obs_seasurfacetemp_delete
public ioda_obs_seasurfacetemp_read, ioda_obs_seasurfacetemp_generate
public ioda_obs_seasurfacetemp_getlocs
public ioda_obs_seasurfacetemp_copy_var

! ------------------------------------------------------------------------------

!> Fortran derived type to hold observation locations
type :: ioda_obs_seasurfacetemp
  integer :: nobs
  real(kind_real), allocatable, dimension(:) :: lat      !< latitude
  real(kind_real), allocatable, dimension(:) :: lon      !< longitude
  real(kind_real), allocatable, dimension(:) :: sst
  real(kind_real), allocatable, dimension(:) :: sst_err  
  integer,         allocatable, dimension(:) :: qc       !< QC flag (from file?)
end type ioda_obs_seasurfacetemp

! ------------------------------------------------------------------------------
contains

! ------------------------------------------------------------------------------

subroutine ioda_obs_seasurfacetemp_setup(self, nobs)
implicit none

type(ioda_obs_seasurfacetemp), intent(inout) :: self
integer, intent(in) :: nobs

call ioda_obs_seasurfacetemp_delete(self)

self%nobs = nobs
allocate(self%lat(nobs), self%lon(nobs))
allocate(self%sst(nobs), self%sst_err(nobs))
allocate(self%qc(nobs))
self%lat = 0.
self%lon = 0.
self%sst = 0.
self%sst_err  = 0.
self%qc = 0

end subroutine ioda_obs_seasurfacetemp_setup

! ------------------------------------------------------------------------------

subroutine ioda_obs_seasurfacetemp_delete(self)
implicit none
type(ioda_obs_seasurfacetemp), intent(inout) :: self

self%nobs = 0
if (allocated(self%lat)) deallocate(self%lat)
if (allocated(self%lon)) deallocate(self%lon)
if (allocated(self%sst)) deallocate(self%sst)
if (allocated(self%sst_err)) deallocate(self%sst_err)
if (allocated(self%qc))  deallocate(self%qc)

end subroutine ioda_obs_seasurfacetemp_delete

! ------------------------------------------------------------------------------

subroutine ioda_obs_seasurfacetemp_generate(self, nobs, lat, lon1, lon2)
implicit none
type(ioda_obs_seasurfacetemp), intent(inout) :: self
integer, intent(in) :: nobs
real, intent(in) :: lat, lon1, lon2

integer :: i

call ioda_obs_seasurfacetemp_setup(self, nobs)
self%nobs=nobs
 self%qc(:)= 1
 self%sst(:) = 25.0
 self%sst_err(:) = 0.1
 self%lat(:)     = lat
 do i = 1, nobs
   self%lon(i) = lon1 + (i-1)*(lon2-lon1)/(nobs-1)
 enddo
! print *, 'in random:', self%nobs, self%lon, self%lat

end subroutine ioda_obs_seasurfacetemp_generate


! ------------------------------------------------------------------------------
! ------------------------------------------------------------------------------

subroutine check(status)
  use netcdf
  integer, intent(in) :: status
  if(status /=nf90_noerr) then
     print *, status
     print *, trim(nf90_strerror(status))
     stop 1
  end if
end subroutine check



subroutine ioda_obs_seasurfacetemp_read(filename, self)
use nc_diag_read_mod, only: nc_diag_read_get_var, nc_diag_read_get_dim
use nc_diag_read_mod, only: nc_diag_read_init, nc_diag_read_close, nc_diag_read_get_attr
use netcdf
implicit none
character(max_string), intent(in)   :: filename
type(ioda_obs_seasurfacetemp), intent(inout) :: self

real(kind_real) :: scale, offset
integer :: iunit, ni,nj, qcnobs, nobs
real, allocatable    :: tmp_lon(:,:), tmp_lat(:,:), tmp_sst(:,:), &     
     tmp_bias(:,:), tmp_err(:,:), tmp_qc(:,:)
integer(2), allocatable :: tmp_int(:,:,:)
integer :: i, qci, ii, jj, vid, ncid, id_x, id_y, j

call ioda_obs_seasurfacetemp_delete(self)


! open netcdf file and read dimensions
call check( nf90_open(filename, nf90_nowrite, ncid) )
call check( nf90_inq_dimid(ncid, "ni", id_x))
call check( nf90_inquire_dimension(ncid, id_x, len=ni))
call check( nf90_inq_dimid(ncid, "nj", id_y))
call check( nf90_inquire_dimension(ncid, id_y, len=nj))
print *, 'ni,nj:', ni, nj
allocate(tmp_lat(ni,nj), tmp_lon(ni,nj), tmp_sst(ni,nj), tmp_bias(ni,nj), tmp_err(ni,nj), tmp_qc(ni,nj))

call check( nf90_inq_varid(ncid, "lat", vid))
call check( nf90_get_var(ncid, vid, tmp_lat))

call check( nf90_inq_varid(ncid, "lon", vid))
call check( nf90_get_var(ncid, vid, tmp_lon))

call check( nf90_inq_varid(ncid, "sea_surface_temperature", vid))
call check( nf90_get_att(ncid, vid, "add_offset", offset))
call check( nf90_get_att(ncid, vid, "scale_factor", scale))
call check( nf90_get_var(ncid, vid, tmp_sst))
tmp_sst = tmp_sst*scale + offset

call check( nf90_inq_varid(ncid, "sses_bias", vid))
call check( nf90_get_att(ncid, vid, "add_offset", offset))
call check( nf90_get_att(ncid, vid, "scale_factor", scale))
call check( nf90_get_var(ncid, vid, tmp_bias))
tmp_bias = tmp_bias*scale + offset

call check( nf90_inq_varid(ncid, "sses_standard_deviation", vid))
call check( nf90_get_att(ncid, vid, "add_offset", offset))
call check( nf90_get_att(ncid, vid, "scale_factor", scale))
call check( nf90_get_var(ncid, vid, tmp_err))
tmp_err = tmp_err*scale + offset

call check( nf90_inq_varid(ncid, "quality_level", vid))
call check( nf90_get_var(ncid, vid, tmp_qc))
where (tmp_qc < 5) tmp_qc = 0
where (tmp_qc == 5) tmp_qc = 1

call check( nf90_close(ncid))

nobs = ni * nj
qcnobs=sum(tmp_qc)
self%nobs = qcnobs

print *,'QC:',nobs,qcnobs

! allocate geovals structure
call ioda_obs_seasurfacetemp_setup(self, qcnobs)

qci = 0
do j = 1, nj
   do i = 1, ni
      if ( tmp_qc(i,j) .eq.1 ) then
         qci = qci + 1
         self%lat(qci) = tmp_lat(i,j)
         self%lon(qci) = tmp_lon(i,j)      
         self%sst(qci) = tmp_sst(i,j)-273.15-tmp_bias(i,j)
         self%sst_err(qci) = tmp_err(i,j)
         write(601,*)self%lon(qci),self%lat(qci),self%sst(qci)
      end if
   end do
end do
self%qc = 1


end subroutine ioda_obs_seasurfacetemp_read

! ------------------------------------------------------------------------------

subroutine ioda_obs_seasurfacetemp_getlocs(self, locs)
use ioda_locs_mod
implicit none
type(ioda_obs_seasurfacetemp), intent(in) :: self
type(ioda_locs), intent(inout) :: locs

print *, "DEBUG, getlocs"
call ioda_locs_setup(locs, self%nobs)
locs%lat = self%lat
locs%lon = self%lon
locs%time = 0

end subroutine ioda_obs_seasurfacetemp_getlocs

! ------------------------------------------------------------------------------

subroutine ioda_obs_seasurfacetemp_copy_var(self, vname, vdata, vsize)
  implicit none

  type(ioda_obs_seasurfacetemp), intent(in) :: self
  character(len=*), intent(in)              :: vname
  real(kind_real), intent(out)              :: vdata(vsize)
  integer, intent(in)                       :: vsize

  character(max_string) :: err_msg

  if (trim(vname) .eq. "latitude") then
    vdata = self%lat
  elseif (trim(vname) .eq. "longitude") then
    vdata = self%lon
  elseif (trim(vname) .eq. "sst") then
    vdata = self%sst
  elseif (trim(vname) .eq. "sst_err") then
    vdata = self%sst_err
  else
    write(err_msg,*) 'ioda_obs_seasurfacetemp_copy_var: Unrecognized variable name: ', trim(vname)
    call abor1_ftn(trim(err_msg))
  endif

end subroutine ioda_obs_seasurfacetemp_copy_var

! ------------------------------------------------------------------------------

end module ioda_obs_seasurfacetemp_mod
