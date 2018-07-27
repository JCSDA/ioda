! (C) Copyright 2018 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran program to convert the existing NetCDF file into an ODB-2

program netcdf_to_odb2

use netcdf, only: &
  nf90_close,     &
  nf90_get_att,   &
  nf90_global,    &
  nf90_noerr,     &
  nf90_nowrite,   &
  nf90_open,      &
  nf90_strerror

use odb_c_binding, only: &
  odb_integer,           &
  odb_real

use odb_helper_mod, only: &
  create_table_sql,       &
  insert_into_sql

use odbql_wrappers, only: &
  odbql,                  &
  odbql_bind_double,      &
  odbql_bind_int,         &
  odbql_close,            &
  odbql_finalize,         &
  odbql_float,            &
  odbql_integer,          &
  odbql_open,             &
  odbql_prepare_v2,       &
  odbql_step,             &
  odbql_stmt

use, intrinsic :: iso_c_binding, only: &
  c_double,                            &
  c_int

implicit none

character(len=1000)         :: filename
character(len=1000)         :: output_filename
integer                     :: rc
integer                     :: file_id
character(len=7000)         :: messages(4)
real, allocatable           :: latitudes(:)
real, allocatable           :: longitudes(:)
real, allocatable           :: times_netcdf(:)
real, allocatable           :: observations(:)
real, allocatable           :: pressures(:)
integer, allocatable        :: times(:)
integer, allocatable        :: dates(:)
type (odbql)                :: odb
integer(kind=c_int)         :: odb_rc
character(len=*), parameter :: odb_column_names(9) = [character(len=18) :: "seqno", "lat", "lon", "entryno", &
                                                                           "date", "time", "varno", "vertco_reference_1", "obsvalue"]
integer                     :: odb_column_types(9) = [odb_integer, odb_real, odb_real, odb_integer, odb_integer, &
                                                      odb_integer, odb_integer, odb_real, odb_real]
character(len=5000)         :: create_table_sql_string
character(len=5000)         :: insert_into_sql_string
character(len=5000)         :: unparsed_sql
type (odbql_stmt)           :: stmt
integer(kind=c_int)         :: col_name_index
integer                     :: iob
real(kind=c_double)         :: buffer(size(odb_column_names))
integer                     :: date_time
integer                     :: arg_num
character(len=2000)         :: arguments(2)

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

call odbql_open ("", odb, odb_rc)
if (odb_rc /= 0) then
  write (messages(1), '(a,i0)') "odbql_open failed with rc = ", odb_rc
  call fail (messages(1:1))
end if

call create_table_sql (odb_column_names, odb_column_types, output_filename, create_table_sql_string)

call odbql_prepare_v2 (odb, trim (create_table_sql_string), -1_c_int, stmt, unparsed_sql, odb_rc)
if (odb_rc /= 0) then
  write (messages(1), '(a,i0)') "odbql_prepare_v2 failed (create table), rc = ", odb_rc
  messages(2) = create_table_sql_string
  call fail (messages(1:2))
end if

call insert_into_sql (odb_column_names, insert_into_sql_string)

call odbql_prepare_v2 (odb, trim (insert_into_sql_string), -1_c_int, stmt, unparsed_sql, odb_rc)
if (odb_rc /= 0) then
  write (messages(1), '(a,i0)') "odbql_prepare_v2 failed (insert into), rc = ", odb_rc
  messages(2) = insert_into_sql_string
  call fail (messages(1:2))
end if

do iob = 1, size(latitudes)
  buffer(1) = iob
  buffer(2) = latitudes(iob)
  buffer(3) = longitudes(iob)
  buffer(5) = dates(iob)
  buffer(6) = times(iob)
  buffer(7) = 2
  buffer(4) = 1
  buffer(8) = pressures(iob)
  buffer(9) = observations(iob)
  do col_name_index = 1, size (odb_column_names)
    select case (odb_column_types(col_name_index))
      case (odbql_float)
        call odbql_bind_double (stmt, col_name_index, buffer(col_name_index))
      case (odbql_integer)
        call odbql_bind_int (stmt, col_name_index, nint (buffer(col_name_index), kind = c_int))
    end select
  end do
  call odbql_step (stmt)
end do

call odbql_finalize (stmt)

call odbql_close (odb)

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

end program netcdf_to_odb2
