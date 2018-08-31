! (C) Copyright 2018 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran program to convert the existing NetCDF file into an ODB-1

program netcdf_to_odb

use netcdf, only: &
  nf90_close,     &
  nf90_get_att,   &
  nf90_global,    &
  nf90_noerr,     &
  nf90_nowrite,   &
  nf90_open,      &
  nf90_strerror

use odb_module, only: &
  odb_close,          &
  odb_end,            &
  odb_getsize,        &
  odb_open,           &
  odb_put

use, intrinsic :: iso_c_binding, only: &
  c_double,                            &
  c_int,                               &
  c_null_char

implicit none

character(len=1000)              :: filename
character(len=1000)              :: output_filename
integer                          :: rc
integer                          :: file_id
character(len=7000)              :: messages(4)
real, allocatable                :: latitudes(:)
real, allocatable                :: longitudes(:)
real, allocatable                :: times_netcdf(:)
real, allocatable                :: observations(:)
real, allocatable                :: pressures(:)
integer, allocatable             :: times(:)
integer, allocatable             :: dates(:)
integer(kind=c_int)              :: odb_rc
integer                          :: iob
integer                          :: date_time
integer                          :: handle
integer                          :: dummy_num_rows
integer                          :: num_columns
real(kind=c_double), allocatable :: desc(:,:)
real(kind=c_double), allocatable :: hdr(:,:)
real(kind=c_double), allocatable :: body(:,:)
integer                          :: num_pools
integer                          :: pool_number

interface
  function setenv (envname, envval, overwrite) bind (c, name = "setenv")
    use, intrinsic :: iso_c_binding, only: &
      c_char,                              &
      c_int
    character(kind=c_char)     :: envname(*)
    character(kind=c_char)     :: envval(*)
    integer(kind=c_int), value :: overwrite
    integer(kind=c_int)        :: setenv
  end function setenv
end interface

call get_command_argument (1, filename)
call get_command_argument (2, output_filename)

if (filename == "" .or. output_filename == "") then
  write (messages(1), '(a,i0)') "Please provide two arguments, input-filename output-filename"
  call fail (messages(1:1))
end if

rc = nf90_open (filename, nf90_nowrite, file_id)
if (rc /= nf90_noerr) then
  write (messages(1), '(a,i0)') "nf90_open failed with rc = ", rc
  write (messages(2), '(2a)') "file name = ", trim (filename)
  messages(3) = nf90_strerror (rc)
  call fail (messages(1:3))
end if

call get_netcdf_array (file_id, 'Latitude', latitudes)
call get_netcdf_array (file_id, 'Longitude', longitudes)
call get_netcdf_array (file_id, 'Time', times_netcdf)

allocate (times(size(times_netcdf)))
allocate (dates(size(times_netcdf)))

rc = nf90_get_att (file_id, nf90_global, "date_time", date_time)
if (rc /= nf90_noerr) then
  write (messages(1), '(a,i0)') "nf90_get_at failed for date_time with rc = ", rc
  write (messages(2), '(a,i0)') "file id = ", file_id
  messages(3) = nf90_strerror (rc)
  call fail (messages(1:3))
end if

dates = date_time / 100
times = (date_time - (date_time / 100) * 100) * 10000
times = times + 10000 * int (times_netcdf)
times_netcdf = times_netcdf - int (times_netcdf)
where (times_netcdf >= 0)
  times = times + nint (60 * times_netcdf) * 100
elsewhere
  times = times - 10000
  times = times + nint (60 * (1 - abs(times_netcdf))) * 100
end where

call get_netcdf_array (file_id, 'Observation', observations)
call get_netcdf_array (file_id, 'Pressure', pressures)

rc = nf90_close (file_id)
if (rc /= nf90_noerr) then
  write (messages(1), '(a,i0)') "nf90_close failed with rc = ", rc
  write (messages(2), '(a,i0)') "file id = ", file_id
  messages(3) = nf90_strerror (rc)
  call fail (messages(1:3))
end if

odb_rc = setenv ("ODB_DATAPATH_OOPS" // c_null_char,   &
                 trim(output_filename) // c_null_char, &
                 1_c_int)

odb_rc = setenv ("ODB_SRCPATH_OOPS" // c_null_char,    &
                 trim(output_filename) // c_null_char, &
                 1_c_int)

odb_rc = setenv ("IOASSIGN" // c_null_char,                           &
                 trim(output_filename) // "/IOASSIGN" // c_null_char, &
                 1_c_int)

call execute_command_line ("unset ODB_COMPILER_FLAGS && &
                           &create_ioassign -l OOPS -d " // trim(output_filename))

num_pools = 1
handle = odb_open ("OOPS", "NEW", num_pools)

odb_rc = odb_getsize (handle, "@desc", dummy_num_rows, num_columns)
allocate (desc(1,0:num_columns))
desc = -32768
odb_rc = odb_getsize (handle, "@hdr", dummy_num_rows, num_columns)
allocate (hdr(size(latitudes),0:num_columns))
hdr = -32768
odb_rc = odb_getsize (handle, "@body", dummy_num_rows, num_columns)
allocate (body(size(latitudes),0:num_columns))
body = -32768

desc(:,1) = 20160308
desc(:,2) = 1200
desc(:,3) = 0
desc(:,4) = size(latitudes)

do iob = 1, size(latitudes)
  hdr(iob,1) = iob
  hdr(iob,2) = dates(iob)
  hdr(iob,3) = times(iob)
  hdr(iob,4) = latitudes(iob)
  hdr(iob,5) = longitudes(iob)
  hdr(iob,6) = iob - 1
  hdr(iob,7) = 1
end do

do iob = 1, size(latitudes)
  body(iob,1) = 2
  body(iob,2) = observations(iob)
  body(iob,3) = 1
  body(iob,4) = pressures(iob)
end do

pool_number = 1
num_columns = size(desc,dim=2)-1
odb_rc = odb_put (handle, "@desc", desc, 1, poolno = pool_number)
num_columns = size(hdr,dim=2)-1
odb_rc = odb_put (handle, "@hdr", hdr, size(latitudes), poolno = pool_number)
num_columns = size(body,dim=2)-1
odb_rc = odb_put (handle, "@body", body, size(latitudes), poolno = pool_number)

odb_rc = odb_close (handle, save = .true.)
odb_rc = odb_end()

contains

subroutine fail (messages)

character(len=*), intent(in) :: messages(:)

integer                      :: i

do i = 1, size(messages)
  write (0, '(a)') trim(messages(i))
  write (*, '(a)') trim(messages(i))
end do
call exit (1)

end subroutine fail

subroutine get_netcdf_array (file_id, var_name, output_array)

use netcdf, only:         &
  nf90_get_var,           &
  nf90_inq_varid,         &
  nf90_inquire_dimension, &
  nf90_inquire_variable,  &
  nf90_noerr,             &
  nf90_strerror

integer, intent(in)            :: file_id
character(len=*), intent(in)   :: var_name
real, allocatable, intent(out) :: output_array(:)

integer                        :: var_id, rc, ndims, length, dimids(1)
character(len=7000)            :: messages(5)

rc = nf90_inq_varid (file_id, var_name, var_id)
if (rc /= nf90_noerr) then
  write (messages(1), '(a,i0)') "nf90_inq_varid failed for " // trim (var_name) // " with rc = ", rc
  write (messages(2), '(a,i0)') "file id = ", file_id
  messages(3) = nf90_strerror (rc)
  call fail (messages(1:3))
end if

rc = nf90_inquire_variable (file_id, var_id, ndims = ndims, dimids = dimids)
if (rc /= nf90_noerr) then
  write (messages(1), '(a,i0)') "nf90_inquire_variable failed for " // trim (var_name) // " with rc = ", rc
  write (messages(2), '(a,i0)') "file id = ", file_id
  write (messages(3), '(a,i0)') "var id = ", var_id
  messages(4) = nf90_strerror (rc)
  call fail (messages(1:4))
end if

if (ndims /= 1) then
  write (messages(1), '(a,i0)') "Should be a 1d arrays for " // trim (var_name)
  write (messages(2), '(a,i0,a)') "Actually got ", ndims, "d array"
  write (messages(3), '(a,i0)') "file id = ", file_id
  write (messages(4), '(a,i0)') "var id = ", var_id
  call fail (messages(1:4))
end if

rc = nf90_inquire_dimension (file_id, dimids(1), len = length)
if (rc /= nf90_noerr) then
  write (messages(1), '(a,i0)') "nf90_inquire_dimension failed for " // trim (var_name) // " with rc = ", rc
  write (messages(2), '(a,i0)') "file id = ", file_id
  write (messages(3), '(a,i0)') "var id = ", var_id
  write (messages(4), '(a,i0)') "dim id = ", dimids(1)
  messages(5) = nf90_strerror (rc)
  call fail (messages(1:5))
end if

allocate (output_array(length))

rc = nf90_get_var (file_id, var_id, output_array)
if (rc /= nf90_noerr) then
  write (messages(1), '(a,i0)') "nf90_get_var failed for " // trim (var_name) // " with rc = ", rc
  write (messages(2), '(a,i0)') "file id = ", file_id
  write (messages(2), '(a,i0)') "var id = ", var_id
  messages(4) = nf90_strerror (rc)
  call fail (messages(1:4))
end if

end subroutine get_netcdf_array

end program netcdf_to_odb
