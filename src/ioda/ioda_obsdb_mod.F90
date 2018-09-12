!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran module handling radiosonde observation space

module ioda_obsdb_mod

use iso_c_binding
use kinds
use ioda_obsvar_mod
use ioda_obs_vectors
use fckit_log_module, only : fckit_log
#ifdef HAVE_ODB_API
use odb_helper_mod, only: &
  count_query_results
#endif

implicit none
private
integer, parameter :: max_string=800

public ioda_obsdb
public ioda_obsdb_setup
public ioda_obsdb_delete
public ioda_obsdb_getlocs
public ioda_obsdb_generate
public ioda_obsdb_var_to_ovec
public ioda_obsdb_putvar

! ------------------------------------------------------------------------------

!> Fortran derived type to hold a set of observation variables
type :: ioda_obsdb
  integer :: fvlen                     !< length of vectors in the input file
  integer :: nobs                      !< number of observations for this process
  integer :: nlocs                     !< number of locations for this process
  integer :: nvars                     !< number of variables
  integer, allocatable :: dist_indx(:) !< indices to select elements from input file vectors

  character(len=max_string) :: obstype

  character(len=max_string) :: filename

  character(len=max_string) :: fileout

  type(ioda_obs_variables) :: obsvars  !< observation variables
#ifdef HAVE_ODB
  real(kind=c_double), allocatable :: odb_data(:,:)
#endif
end type ioda_obsdb

contains

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_setup(self, fvlen, nobs, dist_indx, nlocs, nvars, filename, fileout, obstype)
implicit none
type(ioda_obsdb), intent(inout) :: self
integer, intent(in) :: fvlen
integer, intent(in) :: nobs
integer, intent(in) :: dist_indx(:)
integer, intent(in) :: nlocs
integer, intent(in) :: nvars
character(len=*), intent(in) :: filename
character(len=*), intent(in) :: fileout
character(len=*), intent(in) :: obstype

self%fvlen     = fvlen
self%nobs      = nobs
allocate(self%dist_indx(nobs))
self%dist_indx = dist_indx
self%nlocs     = nlocs
self%nvars     = nvars
self%filename  = filename
self%fileout   = fileout
self%obstype   = obstype
call self%obsvars%setup()

end subroutine ioda_obsdb_setup

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_delete(self)
implicit none
type(ioda_obsdb), intent(inout) :: self

call ioda_obsdb_write(self)

self%fvlen = 0
self%nobs  = 0
if (allocated(self%dist_indx)) deallocate(self%dist_indx)
self%nlocs  = 0
self%nvars  = 0
self%filename = ""
self%obstype = ""

call self%obsvars%delete()

end subroutine ioda_obsdb_delete

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_getlocs(self, locs)
use ioda_locs_mod
implicit none
type(ioda_obsdb), intent(in) :: self
type(ioda_locs), intent(inout) :: locs

character(len=*),parameter:: myname = "ioda_obsdb_getlocs"
character(len=255) :: record
integer :: failed
type(ioda_obs_var), pointer :: vptr
integer :: istep

!Setup ioda locations
call ioda_locs_setup(locs, self%nlocs)


! Assume that obs are organized with the first location going with the
! first nobs/nlocs obs, the second location going with the next nobs/nlocs
! obs, etc. 
istep = self%nobs / self%nlocs 

if ((trim(self%obstype) .eq. "Radiance") .or. &
    (trim(self%obstype) .eq. "Radiosonde") .or. &
    (trim(self%obstype) .eq. "Aircraft")) then
  call ioda_obsdb_getvar(self, "longitude", vptr)
  locs%lon = vptr%vals
else
  call ioda_obsdb_getvar(self, "Longitude", vptr)
  locs%lon = vptr%vals(1:self%nobs:istep)
endif

if ((trim(self%obstype) .eq. "Radiance") .or. &
    (trim(self%obstype) .eq. "Radiosonde") .or. &
    (trim(self%obstype) .eq. "Aircraft")) then
  call ioda_obsdb_getvar(self, "latitude", vptr)
  locs%lat = vptr%vals
else
  call ioda_obsdb_getvar(self, "Latitude", vptr)
  locs%lat = vptr%vals(1:self%nobs:istep)
endif

if ((trim(self%obstype) .eq. "Radiance") .or. &
    (trim(self%obstype) .eq. "Radiosonde") .or. &
    (trim(self%obstype) .eq. "Aircraft")) then
  call ioda_obsdb_getvar(self, "time", vptr)
  locs%time = vptr%vals
elseif (trim(self%obstype) .eq. "Aod") then
  call ioda_obsdb_getvar(self, "Obs_Time", vptr)
  locs%time = vptr%vals(1:self%nobs:istep)
else
  call ioda_obsdb_getvar(self, "Time", vptr)
  locs%time = vptr%vals(1:self%nobs:istep)
endif

write(record,*) myname,': allocated/assinged obs-data'
call fckit_log%info(record)

end subroutine ioda_obsdb_getlocs

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_getvar(self, vname, vptr)

#ifdef HAVE_ODB_API

use odb_helper_mod, only: get_vars
use mpi, only: mpi_comm_rank, mpi_comm_size, mpi_comm_world, mpi_init, mpi_finalize

#endif

use netcdf, only: NF90_FLOAT, NF90_DOUBLE
use nc_diag_read_mod, only: nc_diag_read_init
use nc_diag_read_mod, only: nc_diag_read_get_dim
use nc_diag_read_mod, only: nc_diag_read_check_var
use nc_diag_read_mod, only: nc_diag_read_get_var_dims
use nc_diag_read_mod, only: nc_diag_read_get_var_type
use nc_diag_read_mod, only: nc_diag_read_get_var
use nc_diag_read_mod, only: nc_diag_read_close

implicit none

type(ioda_obsdb), intent(in) :: self
character(len=*), intent(in)   :: vname
type(ioda_obs_var), pointer :: vptr

integer :: iunit
character(len=max_string) :: err_msg
integer :: i
integer :: j
integer :: iflat
integer :: ndims
integer, allocatable :: dimsizes(:)
real, allocatable :: field1d_sngl(:)
real, allocatable :: field2d_sngl(:,:)
real(kind_real), allocatable :: field1d_dbl(:)
real(kind_real), allocatable :: field2d_dbl(:,:)
integer :: input_file_type
integer :: vartype

! Look for the variable. If it is already present, vptr will be
! pointing to it. If not, vptr will not be associated, and we
! need to create the new variable and read in the data from the
! netcdf file.
call self%obsvars%get_node(vname, vptr)
if (.not.associated(vptr)) then
  call self%obsvars%add_node(vname, vptr)

  if (self % filename(len_trim(self % filename)-3:len_trim(self % filename)) == ".nc4" .or. &
      self % filename(len_trim(self % filename)-3:len_trim(self % filename)) == ".nc") then
    input_file_type = 0
  else if (self % filename(len_trim(self % filename)-3:len_trim(self % filename)) == ".odb") then
    input_file_type = 1
  else
    input_file_type = 2
  end if

  select case (input_file_type)
    case (0)

      ! Open the file, and do some checks
      call nc_diag_read_init(self%filename, iunit)

      ! Does the variable exist?
      if (.not.nc_diag_read_check_var(iunit, vname)) then
        write(err_msg,*) 'ioda_obsdb_getvar: var ', trim(vname), ' does not exist'
        call abor1_ftn(trim(err_msg))
      endif
 
      ! Get the dimension sizes of the variable and use these to allocate
      ! the storage for the variable.
      call nc_diag_read_get_var_dims(iunit, vname, ndims, dimsizes)

      ! Determine the data type (single vs double precision)
      vartype = nc_diag_read_get_var_type(iunit, vname)

      if (ndims .gt. 1) then
        write(err_msg,*) 'ioda_obsdb_getvar: var ', trim(vname), ' must have rank = 1'
        call abor1_ftn(trim(err_msg))
      endif

      if (dimsizes(1) .ne. self%fvlen) then
        write(err_msg,*) 'ioda_obsdb_getvar: var ', trim(vname), ' size (', dimsizes(1), ') must equal fvlen (', self%fvlen, ')'
        call abor1_ftn(trim(err_msg))
      endif

      ! The dimensionality of the variables in the netcdf file match what we want in the obsdb.
      vptr%nobs = self%nobs
      allocate(vptr%vals(vptr%nobs))

      if (vartype == NF90_DOUBLE) then
        allocate(field1d_dbl(dimsizes(1)))
        call nc_diag_read_get_var(iunit, vname, field1d_dbl)
        vptr%vals = field1d_dbl(self%dist_indx)
        deallocate(field1d_dbl)
      elseif (vartype == NF90_FLOAT) then
        allocate(field1d_sngl(dimsizes(1)))
        call nc_diag_read_get_var(iunit, vname, field1d_sngl)
        vptr%vals = field1d_sngl(self%dist_indx)
        deallocate(field1d_sngl)
      endif

      call nc_diag_read_close(self%filename)

    case (1)

#ifdef HAVE_ODB_API
      select case (vname)
        case ("Latitude")
          call get_vars (self % filename, ["lat"], "entryno = 1", field2d_dbl)
        case ("Longitude")
          call get_vars (self % filename, ["lon"], "entryno = 1", field2d_dbl)
        case ("Time")
         call get_vars (self % filename, ["time"], "entryno = 1", field2d_dbl)
      end select
      vptr%nobs = size(field2d_dbl,dim=2)
      allocate(vptr%vals(vptr%nobs))
      vptr % vals(:) = field2d_dbl(1,:)
#endif

    case (2)

#ifdef HAVE_ODB
#endif

  end select

endif

end subroutine ioda_obsdb_getvar

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_putvar(self, vname, ovec)

implicit none

type(ioda_obsdb), intent(in) :: self
character(len=*), intent(in) :: vname
type(obs_vector), intent(in) :: ovec

type(ioda_obs_var), pointer  :: vptr
character(len=*),parameter   :: myname = "ioda_obsdb_putvar"
character(len=255)           :: record

call self%obsvars%get_node(vname, vptr)
if (.not.associated(vptr)) then
  call self%obsvars%add_node(vname, vptr)
  vptr%nobs = self%nobs
  allocate(vptr%vals(vptr%nobs))
  vptr%vals = ovec%values 
else
  write(record,*) myname,' var= ', trim(vname), ':this column already exists'
  call fckit_log%info(record)
endif

end subroutine ioda_obsdb_putvar

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_generate(self, fvlen, nobs, dist_indx, nlocs, nvars, obstype, lat, lon1, lon2)
implicit none
type(ioda_obsdb), intent(inout) :: self
integer, intent(in) :: fvlen
integer, intent(in) :: nobs
integer, intent(in) :: dist_indx(:)
integer, intent(in) :: nlocs
integer, intent(in) :: nvars
character(len=*) :: obstype
real, intent(in) :: lat, lon1, lon2

character(len=*),parameter :: myname = "ioda_obsdb_generate"
integer :: i
type(ioda_obs_var), pointer :: vptr
character(len=max_string) :: vname

! 4th argument is the filename containing obs values, which is not used for this method.
call ioda_obsdb_setup(self, fvlen, nobs, dist_indx, nlocs, nvars, "", "", obstype)

! Create variables and generate the values specified by the arguments.
vname = "Latitude" 
call self%obsvars%get_node(vname, vptr)
if (.not.associated(vptr)) then
  call self%obsvars%add_node(vname, vptr)
endif
vptr%nobs = self%nobs
vptr%vals = lat

vname = "Longitude" 
call self%obsvars%get_node(vname, vptr)
if (.not.associated(vptr)) then
  call self%obsvars%add_node(vname, vptr)
endif
vptr%nobs = self%nobs
do i = 1, nobs
  vptr%vals(i) = lon1 + (i-1)*(lon2-lon1)/(nobs-1)
enddo

end subroutine ioda_obsdb_generate

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_var_to_ovec(self, ovec, vname)
use ioda_locs_mod
implicit none
type(ioda_obsdb), intent(in) :: self
type(obs_vector), intent(out) :: ovec
character(len=*), intent(in) :: vname

character(len=*),parameter:: myname = "ioda_obsdb_var_to_ovec"
character(len=255) :: record
type(ioda_obs_var), pointer :: vptr

call ioda_obsdb_getvar(self, vname, vptr)

ovec%values = vptr%vals

end subroutine ioda_obsdb_var_to_ovec

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_write(self)
use netcdf
implicit none

type(ioda_obsdb), intent(in) :: self

type(ioda_obs_var), pointer  :: vptr
character(len=*),parameter   :: myname = "ioda_obsdb_write"
character(len=255)           :: record
integer                      :: i,ncid,dimid_1d(1),dimid_nobs
integer, allocatable         :: ncid_var(:)



if (self%fileout(len_trim(self%fileout)-3:len_trim(self%fileout)) == ".nc4" .or. &
    self%fileout(len_trim(self%fileout)-3:len_trim(self%fileout)) == ".nc") then

   write(record,*) myname, ':write diag in netcdf, filename=', trim(self%fileout)
   call fckit_log%info(record)

   call check('nf90_create', nf90_create(trim(self%fileout),nf90_hdf5,ncid))
   call check('nf90_def_dim', nf90_def_dim(ncid,trim(self%obstype)//'_nobs',self%nobs, dimid_nobs))
   dimid_1d = (/ dimid_nobs /)

   i = 0
   allocate(ncid_var(self%obsvars%n_nodes))
   vptr => self%obsvars%head
   do while (associated(vptr))
      i = i + 1
      call check('nf90_def_var', nf90_def_var(ncid,trim(vptr%vname)//'_'//trim(self%obstype),nf90_double,dimid_1d,ncid_var(i)))
      vptr => vptr%next
   enddo

   call check('nf90_enddef', nf90_enddef(ncid))

   i = 0
   vptr => self%obsvars%head
   do while (associated(vptr))
      i = i + 1
      call check('nf90_put_var', nf90_put_var(ncid,ncid_var(i),vptr%vals))
      vptr => vptr%next
   enddo

   call check('nf90_close', nf90_close(ncid))
   deallocate(ncid_var)

else if (self%fileout(len_trim(self%fileout)-3:len_trim(self%fileout)) == ".odb") then

   write(record,*) myname, ':write diag in odb2, filename=', trim(self%fileout)
   call fckit_log%info(record)

else

   write(record,*) myname, ':no output'
   call fckit_log%info(record)

endif

end subroutine ioda_obsdb_write

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_dump(self)
  implicit none

  type(ioda_obsdb), intent(in) :: self

  type(ioda_obs_var), pointer :: vptr

  print*, "DEBUG: ioda_obsdb_dump: fvlen: ", self%fvlen
  print*, "DEBUG: ioda_obsdb_dump: nobs: ", self%nobs
  print*, "DEBUG: ioda_obsdb_dump: nlocs: ", self%nlocs
  print*, "DEBUG: ioda_obsdb_dump: nvars: ", self%nvars
  print*, "DEBUG: ioda_obsdb_dump: obstype: ", trim(self%obstype)
  print*, "DEBUG: ioda_obsdb_dump: filename: ", trim(self%filename)

  print*, "DEBUG: ioda_obsdb_dump: count: ", self%obsvars%n_nodes

  vptr => self%obsvars%head
  do while (associated(vptr))
    print*, "DEBUG: ioda_obsdb_dump: vname: ", trim(vptr%vname)
    print*, "DEBUG: ioda_obsdb_dump: shape, size: ", shape(vptr%vals), size(vptr%vals)
    print*, "DEBUG: ioda_obsdb_dump: vals (first 5): ", vptr%vals(1:5)
    print*, "DEBUG: ioda_obsdb_dump: vals (last 5): ", vptr%vals(vptr%nobs-4:vptr%nobs)

    vptr => vptr%next
  enddo

  print*, "DEBUG: ioda_obsdb_dump:"

end subroutine ioda_obsdb_dump

! ------------------------------------------------------------------------------
subroutine check(action, status)

use netcdf, only: nf90_noerr, nf90_strerror

implicit none

integer, intent (in) :: status
character (len=*), intent (in) :: action

if(status /= nf90_noerr) then
   print *, "During action: ", trim(action), ", received error: ", trim(nf90_strerror(status))
   stop 2
end if

end subroutine check

end module ioda_obsdb_mod
