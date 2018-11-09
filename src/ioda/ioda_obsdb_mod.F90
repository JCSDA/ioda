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
use obsspace_mod, only: obspace_missing_value
use fckit_log_module, only : fckit_log
#ifdef HAVE_ODB_API
use odb_helper_mod, only: &
  count_query_results
#endif

implicit none
private
integer, parameter :: max_string=512

public ioda_obsdb
public ioda_obsdb_setup
public ioda_obsdb_delete
public ioda_obsdb_get_ftype
public ioda_obsdb_getlocs
public ioda_obsdb_generate
public ioda_obsdb_var_to_ovec
public ioda_obsdb_get_vec
public ioda_obsdb_put_vec
public ioda_obsdb_has

! ------------------------------------------------------------------------------

!> Fortran derived type to hold a set of observation variables
type :: ioda_obsdb
  integer :: fvlen                     !< length of vectors in the input file
  integer :: nobs                      !< number of observations for this process
  integer :: nlocs                     !< number of locations for this process
  integer :: nvars                     !< number of variables
  character(len=max_string), allocatable :: varnames(:) !< list of variables names
  integer, allocatable :: dist_indx(:) !< indices to select elements from input file vectors

  character(len=max_string) :: obstype
  character(len=max_string) :: filename
  character(len=max_string) :: fileout

  real(c_double) :: missing_value

  type(ioda_obs_variables) :: obsvars  !< observation variables

  character(len=max_string), allocatable :: read_only_groups(:)

#ifdef HAVE_ODB
  real(kind=c_double), allocatable :: odb_data(:,:)
#endif
end type ioda_obsdb

contains

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_setup(self, fvlen, nobs, dist_indx, nlocs, nvars, varnames, filename, fileout, obstype, missing_value)

use nc_diag_read_mod, only: nc_diag_read_init
use nc_diag_read_mod, only: nc_diag_read_close
use ncdr_vars, only: nc_diag_read_noid_get_var_names
use ncdr_vars, only: nc_diag_read_ret_var_dims

implicit none

type(ioda_obsdb), intent(inout) :: self
integer, intent(in) :: fvlen
integer, intent(in) :: nobs
integer, intent(in) :: dist_indx(:)
integer, intent(in) :: nlocs
integer, intent(in) :: nvars
character(len=*), intent(in) :: varnames(:)
character(len=*), intent(in) :: filename
character(len=*), intent(in) :: fileout
character(len=*), intent(in) :: obstype
real(c_double), intent(in)   :: missing_value

integer :: input_file_type
integer :: iunit
character(len=:), allocatable :: var_list(:)
logical, allocatable :: var_select(:)
integer :: file_nvars
integer :: var_list_item_len
integer :: i
integer(selected_int_kind(8)), allocatable :: dim_sizes(:)
type(ioda_obs_var), pointer :: vptr

character(len=max_string) :: gname
character(len=max_string) :: prev_gname
integer :: ngroups

self%fvlen         = fvlen
self%nobs          = nobs
allocate(self%dist_indx(nobs))
self%dist_indx     = dist_indx
self%nlocs         = nlocs
self%nvars         = nvars
allocate(self%varnames(nvars))
self%filename      = filename
self%fileout       = fileout
self%obstype       = obstype
self%missing_value = missing_value
call self%obsvars%setup()

input_file_type = ioda_obsdb_get_ftype(self%filename)
select case (input_file_type)
 case (0)
  ! We want the new C++ ObsSpace to read in the contents of the input file
  ! during the constructor. For testing purposes, do that now using the 
  ! ioda_obsdb_getvar interface. The ioda_obsdb_getvar interface opens the file,
  ! so at this point construct a list of variable names to read in, then close
  ! the file, and then read in the variables using ioda_obsdb_getvar.
  call nc_diag_read_init(self%filename, iunit)

  call nc_diag_read_noid_get_var_names(num_vars=file_nvars, var_name_mlen=var_list_item_len)
  allocate(character(var_list_item_len)::var_list(file_nvars))
  call nc_diag_read_noid_get_var_names(var_names=var_list)

  allocate(var_select(file_nvars))
  do i = 1, file_nvars
    ! Only read in the 1D (vector) arrays that match the fvlen size
    dim_sizes = nc_diag_read_ret_var_dims(trim(var_list(i)))
    var_select(i) = (size(dim_sizes) .eq. 1) .and. (dim_sizes(1) .eq. fvlen)
    deallocate(dim_sizes)
  enddo

  call nc_diag_read_close(self%filename)

 case (1)

  file_nvars = 3
  var_list_item_len = 80
  allocate(character(var_list_item_len)::var_list(file_nvars))
  allocate(var_select(file_nvars))

  var_list(1) = "latitude"
  var_list(2) = "longitude"
  var_list(3) = "time"

  var_select(1) = .true.
  var_select(2) = .true.
  var_select(2) = .true.

 case (2)

endselect

! Read in the selected file variables
prev_gname = ""
ngroups = 0
do i = 1, file_nvars
  if (var_select(i)) then
    call ioda_obsdb_getvar(self, trim(var_list(i)), vptr)

    ! Extract the group name, and count up the unique number of groups
    call extract_group_name(var_list(i), gname)
    if (trim(prev_gname) .ne. trim(gname)) then
      ngroups = ngroups + 1
      prev_gname = gname
    endif
  endif
enddo

! Loop through the var_list again and store the unique groups name in the data member
allocate(self%read_only_groups(ngroups))
prev_gname = ""
ngroups = 0
do i = 1, file_nvars
  if (var_select(i)) then
    ! Extract the group name, and record the unique number of groups
    call extract_group_name(var_list(i), gname)
    if (trim(prev_gname) .ne. trim(gname)) then
      ngroups = ngroups + 1
      self%read_only_groups(ngroups) = gname
      prev_gname = gname
    endif
  endif
enddo

deallocate(var_list)
deallocate(var_select)

end subroutine ioda_obsdb_setup

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_delete(self)
implicit none
type(ioda_obsdb), intent(inout) :: self

call ioda_obsdb_write(self)

self%fvlen = 0
self%nobs  = 0
if (allocated(self%dist_indx)) deallocate(self%dist_indx)
if (allocated(self%read_only_groups)) deallocate(self%read_only_groups)
self%nlocs  = 0
self%nvars  = 0
self%filename = ""
self%obstype = ""

call self%obsvars%delete()

end subroutine ioda_obsdb_delete

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_getlocs(self, locs, t1, t2)
use ioda_locs_mod
use datetime_mod
use duration_mod
implicit none
type(ioda_obsdb), intent(in)    :: self
type(ioda_locs),  intent(inout) :: locs
type(datetime),   intent(in)    :: t1, t2

character(len=*),parameter:: myname = "ioda_obsdb_getlocs"
character(len=255) :: record
integer :: failed
type(ioda_obs_var), pointer :: vptr
integer :: istep
integer :: ilocs, i
integer, dimension(:), allocatable :: indx
real(kind_real), dimension(:), allocatable :: time, lon, lat
type(duration), dimension(:), allocatable :: dt
type(datetime), dimension(:), allocatable :: t
type(datetime) :: reftime
  
character(21) :: tstr, tstr2

! Assume that obs are organized with the first location going with the
! first nobs/nlocs obs, the second location going with the next nobs/nlocs
! obs, etc.
istep = self%nobs / self%nlocs

! Local copies pre binning
allocate(time(self%nlocs), lon(self%nlocs), lat(self%nlocs))
allocate(indx(self%nlocs))

if ((trim(self%obstype) .eq. "Radiosonde") .or. &
    (trim(self%obstype) .eq. "Aircraft")) then
  call ioda_obsdb_getvar(self, "longitude@MetaData", vptr)
else
  call ioda_obsdb_getvar(self, "longitude", vptr)
endif
lon = vptr%vals

if ((trim(self%obstype) .eq. "Radiosonde") .or. &
    (trim(self%obstype) .eq. "Aircraft")) then
  call ioda_obsdb_getvar(self, "latitude@MetaData", vptr)
else
  call ioda_obsdb_getvar(self, "latitude", vptr)
endif
lat = vptr%vals

if ((trim(self%obstype) .eq. "Radiosonde") .or. &
    (trim(self%obstype) .eq. "Aircraft")) then
  call ioda_obsdb_getvar(self, "time@MetaData", vptr)
else
  call ioda_obsdb_getvar(self, "time", vptr)
endif
time = vptr%vals


allocate(dt(self%nlocs), t(self%nlocs))

!Time coming from file is integer representing distance to time in file name
!Use hardcoded date for now but needs to come from ObsSpace somehow.
!AS: not going to fix it for now, will wait for either suggestions or new IODA.
call datetime_create("2018-04-15T00:00:00Z", reftime)

call datetime_to_string(reftime, tstr)

do i = 1, self%nlocs
  dt(i) = int(3600*time(i))
  t(i) = reftime
  call datetime_update(t(i), dt(i))
enddo

call datetime_to_string(t1,tstr)
call datetime_to_string(t2,tstr2)

! Find number of locations in this timeframe
ilocs = 0
do i = 1, self%nlocs
  if (t(i) > t1 .and. t(i) <= t2) then
    ilocs = ilocs + 1
    indx(ilocs) = i
    call datetime_to_string(t(i),tstr)
  endif
enddo

deallocate(dt, t)

!Setup ioda locations
call ioda_locs_setup(locs, ilocs)
do i = 1, ilocs
  locs%lon(i)  = lon(indx(i))
  locs%lat(i)  = lat(indx(i))
  locs%time(i) = time(indx(i))
enddo
locs%indx = indx(1:ilocs)

deallocate(time, lon, lat)
deallocate(indx)

write(record,*) myname,': allocated/assinged obs-data'
call fckit_log%info(record)

end subroutine ioda_obsdb_getlocs

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_getvar(self, vname, vptr)

#ifdef HAVE_ODB_API

use odb_helper_mod, only: get_vars
use mpi, only: mpi_comm_rank, mpi_comm_size, mpi_comm_world, mpi_init, mpi_finalize

#endif

use netcdf, only: NF90_FLOAT, NF90_DOUBLE, NF90_INT
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
integer, allocatable :: field1d_int(:)
integer, allocatable :: field2d_int(:,:)
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

  input_file_type = ioda_obsdb_get_ftype(self%filename)
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
      elseif (vartype == NF90_INT) then
        allocate(field1d_int(dimsizes(1)))
        call nc_diag_read_get_var(iunit, vname, field1d_int)
        vptr%vals = field1d_int(self%dist_indx)
        deallocate(field1d_int)
      endif

      ! set the missing value equal to IODA missing_value
      if (vartype == NF90_DOUBLE .or. vartype == NF90_FLOAT ) then
        where(vptr%vals > 1.0e08) vptr%vals = self%missing_value
      endif

      deallocate(dimsizes)

      call nc_diag_read_close(self%filename)

    case (1)

#ifdef HAVE_ODB_API
      select case (vname)
        case ("latitude")
          call get_vars (self % filename, ["lat"], "entryno = 1", field2d_dbl)
        case ("longitude")
          call get_vars (self % filename, ["lon"], "entryno = 1", field2d_dbl)
        case ("time")
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

  ! Check for missing values and replace with the ObsSpace missing value mark
  do i = 1, size(vptr%vals)
    if (vptr%vals(i) > 1.0e08) then
      vptr%vals(i) = self%missing_value
    endif
  enddo
endif

end subroutine ioda_obsdb_getvar

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_get_vec(self, vname, vdata)
use ioda_locs_mod
implicit none
type(ioda_obsdb), intent(in) :: self
character(len=*), intent(in) :: vname
real(kind_real), intent(out) :: vdata(:)

character(len=*),parameter:: myname = "ioda_obsdb_get_vec"
character(len=255) :: record
type(ioda_obs_var), pointer :: vptr

call ioda_obsdb_getvar(self, vname, vptr)

vdata = vptr%vals

end subroutine ioda_obsdb_get_vec

! ------------------------------------------------------------------------------

integer function ioda_obsdb_has(self, vname)
use ioda_locs_mod
implicit none
type(ioda_obsdb), intent(in) :: self
character(len=*), intent(in) :: vname

ioda_obsdb_has = self%obsvars%has(vname)

end function ioda_obsdb_has

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_put_vec(self, vname, vdata)

implicit none

type(ioda_obsdb), intent(in) :: self
character(len=*), intent(in) :: vname
real(kind_real), intent(in)  :: vdata(:)

type(ioda_obs_var), pointer :: vptr
character(len=*),parameter  :: myname = "ioda_obsdb_put_vec"
character(len=255)          :: record

call self%obsvars%get_node(vname, vptr)
if (.not.associated(vptr)) then
  call self%obsvars%add_node(vname, vptr)
  vptr%nobs = self%nobs
  allocate(vptr%vals(vptr%nobs))
  vptr%vals = vdata 
else
  if (is_read_only(self, vname)) then
    write(record,*) myname, ': ERROR: Cannot write into a read only group: ', trim(vname)
    call abor1_ftn(record)
  else
    write(record,*) myname, ': WARNING: Updating an existing group: ', trim(vname)
    call fckit_log%info(record)
    vptr%vals = vdata
  endif
endif

end subroutine ioda_obsdb_put_vec

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_generate(self, fvlen, nobs, dist_indx, nlocs, nvars, obstype, missing_value, lat, lon1, lon2)
implicit none
type(ioda_obsdb), intent(inout) :: self
integer, intent(in) :: fvlen
integer, intent(in) :: nobs
integer, intent(in) :: dist_indx(:)
integer, intent(in) :: nlocs
integer, intent(in) :: nvars
character(len=*) :: obstype
real(c_double) :: missing_value
real, intent(in) :: lat, lon1, lon2

character(len=*),parameter :: myname = "ioda_obsdb_generate"
integer :: i
type(ioda_obs_var), pointer :: vptr
character(len=max_string) :: vname

call ioda_obsdb_setup(self, fvlen, nobs, dist_indx, nlocs, nvars, (/""/), "", "", obstype, missing_value)

! Create variables and generate the values specified by the arguments.
vname = "latitude" 
call self%obsvars%get_node(vname, vptr)
if (.not.associated(vptr)) then
  call self%obsvars%add_node(vname, vptr)
endif
vptr%nobs = self%nobs
vptr%vals = lat

vname = "longitude" 
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
real(kind=kind_real), intent(inout) :: ovec(:)
character(len=*), intent(in) :: vname

character(len=*),parameter:: myname = "ioda_obsdb_var_to_ovec"
character(len=255) :: record
type(ioda_obs_var), pointer :: vptr

call ioda_obsdb_getvar(self, vname, vptr)

ovec(:) = vptr%vals(:)

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

   write(record,*) myname, ': write diag in odb2, filename=', trim(self%fileout)
   call fckit_log%info(record)

else

   write(record,*) myname, ': no output'
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

! ------------------------------------------------------------------------------
function ioda_obsdb_get_ftype(fname) result(ftype)
  implicit none

  character(len=*), intent(in) :: fname
  integer :: ftype

  ! result goes into ftype
  !   0 - netcdf
  !   1 - ODB API
  !   2 - ODB
  if (fname(len_trim(fname)-3:len_trim(fname)) == ".nc4" .or. &
      fname(len_trim(fname)-2:len_trim(fname)) == ".nc") then
    ftype = 0
  elseif (fname(len_trim(fname)-3:len_trim(fname)) == ".odb") then
    ftype = 1
  else
    ftype = 2
  endif
end function ioda_obsdb_get_ftype

! ------------------------------------------------------------------------------
subroutine extract_group_name(vname, gname)
  implicit none

  character(len=*), intent(in)    :: vname
  character(len=*), intent(inout) :: gname

  integer :: gindex

  ! The group name is the suffix after a '@' character.
  gindex = index(vname, "@")
  if (gindex .ne. 0) then
    gname = vname(gindex+1:)
  else
    gname = "GroupUndefined"
  endif

end subroutine extract_group_name

! ------------------------------------------------------------------------------
logical function is_read_only(self, vname)
  implicit none

  type(ioda_obsdb), intent(in) :: self
  character(len=*), intent(in)    :: vname

  character(len=max_string) :: gname
  integer :: i

  ! check to see if gname exists in the read only list
  call extract_group_name(vname, gname)
  is_read_only = .false.
  do i = 1, size(self%read_only_groups)
    is_read_only = is_read_only .or. (trim(gname) .eq. trim(self%read_only_groups(i)))
  enddo

end function is_read_only

! ------------------------------------------------------------------------------

end module ioda_obsdb_mod
