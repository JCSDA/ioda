! (C) Copyright 2018 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran module with ODB API utility routines

module odb_helper_mod

implicit none

integer, parameter, private :: real32 = selected_real_kind(6)
integer, parameter, private :: real64 = selected_real_kind(15)

contains

subroutine count_query_results (filename,    &
                                query,       &
                                num_results)

use odbql_wrappers, only: &
  odbql,                  &
  odbql_close,            &
  odbql_done,             &
  odbql_finalize,         &
  odbql_open,             &
  odbql_prepare_v2,       &
  odbql_row,              &
  odbql_step,             &
  odbql_stmt

use, intrinsic :: iso_c_binding, only: &
  c_int,                               &
  c_null_char

character(len=*), intent(in) :: filename
character(len=*), intent(in) :: query
integer, intent(out)         :: num_results

type (odbql)                 :: db
integer(kind=c_int)          :: errstat
character(len=500)           :: message
character(len=*),parameter   :: myname = "count_query_results"
type (odbql_stmt)            :: stmt
character(len=1000)          :: unparsed_sql
external                     :: abor1_ftn

call odbql_open (trim(filename) // c_null_char, &
                 db,                            &
                 errstat)

if (errstat /= 0) then
  write (message, '(a,i0)') myname // ": failure in odbql_open when counting for input file " // &
                                 trim(filename) // ", errstat = ", errstat
  call abor1_ftn (message)
end if

call odbql_prepare_v2 (db,           &
                       trim(query),  &
                       -1_c_int,     &
                       stmt,         &
                       unparsed_sql, &
                       errstat)

if (errstat /= 0) then
  write (message, '(a,i0)') myname // ": failure in odbql_prepare_v2 when counting for input file " // &
                                 trim (filename) // ", errstat = ", errstat
  call abor1_ftn (message)
end if

num_results = 0
do
  call odbql_step (stmt, &
                   errstat)
  select case (errstat)
    case (odbql_done)
      exit
    case (odbql_row)
      num_results = num_results + 1
    case default
      write(message, '(a,i0,a,i0)') myname // ": failure in odbql_step when counting for input file " // &
                                      trim (filename) // ", errstat = ", errstat, " row number = ", num_results + 1
      call abor1_ftn (message)
  end select
end do

call odbql_finalize(stmt)
call odbql_close(db)

end subroutine count_query_results

subroutine get_vars (filename, &
                     columns,  &
                     filter,   &
                     data)

use odbql_wrappers, only: &
  odbql,                  &
  odbql_close,            &
  odbql_column_text,      &
  odbql_column_type,      &
  odbql_column_value,     &
  odbql_done,             &
  odbql_finalize,         &
  odbql_float,            &
  odbql_integer,          &
  odbql_open,             &
  odbql_prepare_v2,       &
  odbql_row,              &
  odbql_step,             &
  odbql_stmt,             &
  odbql_text,             &
  odbql_value,            &
  odbql_value_double,     &
  odbql_value_int

use, intrinsic :: iso_c_binding, only: &
  c_int,                               &
  c_null_char

character(len=*), intent(in)                :: filename
character(len=*), intent(in)                :: columns(:)
character(len=*), intent(in)                :: filter
real(kind=real64), allocatable, intent(out) :: data(:,:)

type (odbql)                                :: db
integer(kind=c_int)                         :: errstat
character(len=500)                          :: message
character(len=*),parameter                  :: myname = "get_vars"
external                                    :: abor1_ftn
type (odbql_stmt)                           :: stmt
character(len=1000)                         :: unparsed_sql
integer                                     :: num_results, num_columns, i, j, type
type (odbql_value)                          :: val
character(len=5000)                         :: query
character(len=30)                           :: string_val

call odbql_open (trim (filename) // c_null_char, &
                 db,                             &
                 errstat)

if (errstat /= 0) then
  write (message, '(a,i0)') myname // ": failure in odbql_open when reading for input file " // &
                            trim (filename) // ", errstat = ", errstat
  call abor1_ftn (message)
end if

num_columns = size(columns)

call create_query_sql (columns, filename, filter, query)

call count_query_results (filename, query, num_results)

allocate (data(num_columns,num_results))

call odbql_prepare_v2 (db,           &
                       trim(query),  &
                       -1_c_int,     &
                       stmt,         &
                       unparsed_sql, &
                       errstat)

if (errstat /= 0) then
  write (message, '(a,i0)') myname // ": failure in odbql_prepare_v2 when reading for input file " // &
                            trim (filename) // ", errstat = ", errstat
  call abor1_ftn (message)
end if

do i = 1, num_results
  call odbql_step (stmt,    &
                   errstat)
  select case (errstat)
    case (odbql_row)
      continue
    case (odbql_done)
      write(message, '(a,i0,a,i0)') myname // ": odbql_step exited early reading input file " // &
                                    trim (filename) // " row number = ", i, " out of ", num_results
      call abor1_ftn (message)
    case default
      write(message, '(a,i0,a,i0)') myname // ": failure in odbql_step when reading for input file " // &
                                    trim (filename) // ", errstat = ", errstat, " row number = ", i
      call abor1_ftn (message)
  end select
  do j = 1, num_columns
    val = odbql_column_value (stmt, j)
    select case (odbql_column_type (stmt, j))
      case (odbql_float)
        data(j,i) = odbql_value_double (val)
      case (odbql_integer)
        data(j,i) = odbql_value_int (val)
      case (odbql_text)
        call odbql_column_text (stmt, j, string_val)
        data(j,i) = transfer (string_val(1:8), data(j,i))
      case default
        write (message, '(a,i0,a,i0)') myname // ": invalid column type for row ", i, " and column ", j
        call abor1_ftn (message)
    end select
  end do
end do

call odbql_finalize(stmt)
call odbql_close(db)

end subroutine get_vars

subroutine create_query_sql (column_names, &
                             filename,     &
                             filter,       &
                             sql)

implicit none

character(len=*), intent(in)  :: column_names(:)
character(len=*), intent(in)  :: filename
character(len=*), intent(in)  :: filter
character(len=*), intent(out) :: sql

integer                       :: i
character(len=10000)          :: sql_buffer

sql = "select "
do i = 1, size (column_names)
  sql_buffer = column_names(i)
  if (i < size (column_names)) then
    sql_buffer = trim (sql_buffer) // ","
  end if
  sql = trim (sql) // " " // sql_buffer
end do

sql = trim (sql) // " from '" // trim (filename) // "' where " // trim (filter) // ";"

end subroutine create_query_sql

subroutine create_table_sql (column_names, &
                             column_types, &
                             filename,     &
                             sql)

use odb_c_binding, only: &
  odb_integer,           &
  odb_real,              &
  odb_string

implicit none

character(len=*), intent(in)  :: column_names(:)
integer, intent(in)           :: column_types(:)
character(len=*), intent(in)  :: filename
character(len=*), intent(out) :: sql

character(len=300)            :: messages(3)
integer                       :: i
character(len=10000)          :: sql_buffer

sql = "CREATE TABLE odb AS ("
do i = 1, size (column_types)
  select case (column_types(i))
    case (odb_integer)
      sql_buffer = trim (column_names(i)) // " INTEGER"
    case (odb_real)
      sql_buffer = trim (column_names(i)) // " REAL"
    case (odb_string)
      sql_buffer = TRIM (column_names(i)) // " STRING"
  end select
  if (i < size (column_types)) then
    sql_buffer = trim (sql_buffer) // ","
  end if
  sql = trim (sql) // sql_buffer
end do
sql = trim (sql) // ") ON '" // trim (filename) // "';"

end subroutine create_table_sql

subroutine insert_into_sql (column_names, &
                            sql)

implicit none

character(len=*), intent(in)  :: column_names(:)
character(len=*), intent(out) :: sql

integer                       :: i
character(len=10000)          :: sql_buffer

sql = "INSERT INTO odb ("
do i = 1, size (column_names)
  sql_buffer = column_names(i)
  if (i < size (column_names)) then
    sql_buffer = trim (sql_buffer) // ","
  end if
  sql = trim (sql) // sql_buffer
end do
sql = trim (sql) // ") VALUES (" // repeat ("?,", size (column_names) - 1) // "?);"

end subroutine insert_into_sql

end module odb_helper_mod
