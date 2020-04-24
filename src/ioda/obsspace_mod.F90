!
! (C) Copyright 2017 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Fortran interface to ObsSpace.

module obsspace_mod

use, intrinsic :: iso_c_binding
use kinds
use string_f_c_mod
use datetime_mod

implicit none

private
public obsspace_construct
public obsspace_destruct
public obsspace_obsname
public obsspace_obsvariables
public obsspace_get_gnlocs
public obsspace_get_nlocs
public obsspace_get_nrecs
public obsspace_get_nvars
public obsspace_get_comm
public obsspace_get_recnum
public obsspace_get_index
public obsspace_get_db
public obsspace_put_db
public obsspace_has

#include "obsspace_interface.f"

!-------------------------------------------------------------------------------

interface obsspace_get_db
  module procedure obsspace_get_db_int32
  module procedure obsspace_get_db_int64
  module procedure obsspace_get_db_real32
  module procedure obsspace_get_db_real64
  module procedure obsspace_get_db_datetime
end interface

interface obsspace_put_db
  module procedure obsspace_put_db_int32
  module procedure obsspace_put_db_int64
  module procedure obsspace_put_db_real32
  module procedure obsspace_put_db_real64
end interface

!-------------------------------------------------------------------------------
contains
!-------------------------------------------------------------------------------

type(c_ptr) function obsspace_construct(c_conf, tbegin, tend)
  use fckit_configuration_module, only: fckit_configuration
  use datetime_mod, only: datetime
  implicit none
  type(fckit_configuration), intent(in) :: c_conf
  type(datetime), intent(in) :: tbegin, tend
  type(c_ptr) :: c_tbegin, c_tend

  call f_c_datetime(tbegin, c_tbegin)
  call f_c_datetime(tend, c_tend)
  obsspace_construct = c_obsspace_construct(c_conf%c_ptr(), c_tbegin, c_tend)
end function obsspace_construct

!-------------------------------------------------------------------------------

subroutine obsspace_destruct(c_obss)
  implicit none
  type(c_ptr), intent(inout) :: c_obss

  call c_obsspace_destruct(c_obss)
  c_obss = c_null_ptr
end subroutine obsspace_destruct

!-------------------------------------------------------------------------------

!> Get obsname from ObsSpace

subroutine obsspace_obsname(obss, obsname)
  use string_f_c_mod
  implicit none

  type(c_ptr), value, intent(in) :: obss
  character(*), intent(inout)    :: obsname

  integer(c_size_t) :: lcname

  !< If changing the length of name and cname, need to change the ASSERT in
  !obsspace_f.cc also
  character(kind=c_char,len=1) :: cname(101)

  call c_obsspace_obsname(obss, lcname, cname)
  call c_f_string(cname, obsname)
  obsname = obsname(1:lcname)

end subroutine obsspace_obsname

!-------------------------------------------------------------------------------

!> Get obsvariables from ObsSpace

type(oops_variables) function obsspace_obsvariables(obss)
  use oops_variables_mod
  implicit none
  type(c_ptr), value, intent(in) :: obss

  obsspace_obsvariables = oops_variables(c_obsspace_obsvariables(obss))
end function obsspace_obsvariables

!-------------------------------------------------------------------------------
!>  Return the number of observational locations in the input obs file

integer function obsspace_get_gnlocs(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_gnlocs = c_obsspace_get_gnlocs(c_obss)
end function obsspace_get_gnlocs

!-------------------------------------------------------------------------------
!>  Return the number of observational locations in this ObsSpace object

integer function obsspace_get_nlocs(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_nlocs = c_obsspace_get_nlocs(c_obss)
end function obsspace_get_nlocs

!-------------------------------------------------------------------------------
!>  Return the number of observational records (profiles)

integer function obsspace_get_nrecs(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_nrecs = c_obsspace_get_nrecs(c_obss)
end function obsspace_get_nrecs

!-------------------------------------------------------------------------------
!>  Return the number of observational variables

integer function obsspace_get_nvars(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_nvars = c_obsspace_get_nvars(c_obss)
end function obsspace_get_nvars

!-------------------------------------------------------------------------------
!>  Return the name and name length of obsspace communicator
subroutine obsspace_get_comm(obss, f_comm)
  use fckit_mpi_module, only: fckit_mpi_comm
  use string_f_c_mod
  implicit none

  type(c_ptr), intent(in)          :: obss
  type(fckit_mpi_comm),intent(out) :: f_comm

  integer :: lcname
  !< If changing the length of name and cname, need to change the ASSERT in obsspace_f.cc also
  character(kind=c_char,len=1) :: cname(101)
  character(len=100) :: name
  character(len=:), allocatable :: name_comm

  call c_obsspace_get_comm(obss, lcname, cname)
  call c_f_string(cname, name)

  name_comm = name(1:lcname)
  f_comm = fckit_mpi_comm(name_comm)
end subroutine obsspace_get_comm

!-------------------------------------------------------------------------------
!>  Get the record number vector
subroutine obsspace_get_recnum(obss, recnum)
  implicit none
  type(c_ptr), intent(in)          :: obss
  integer(c_size_t), intent(inout) :: recnum(:)

  integer(c_size_t) :: length

  length = size(recnum)
  call c_obsspace_get_recnum(obss, length, recnum)
end subroutine obsspace_get_recnum

!-------------------------------------------------------------------------------
!>  Get the index vector
subroutine obsspace_get_index(obss, indx)
  implicit none
  type(c_ptr), intent(in)          :: obss
  integer(c_size_t), intent(inout) :: indx(:)

  integer(c_size_t) :: length

  length = size(indx)
  call c_obsspace_get_index(obss, length, indx)
end subroutine obsspace_get_index

!-------------------------------------------------------------------------------

!>  Return true if variable exists in database

logical function obsspace_has(c_obss, group, vname)
  implicit none
  type(c_ptr), intent(in)      :: c_obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)

  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  obsspace_has = c_obsspace_has(c_obss, c_group, c_vname)
end function obsspace_has

!-------------------------------------------------------------------------------

!> Get a variable from the ObsSapce database

subroutine obsspace_get_db_int32(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int32_t), intent(inout) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_get_int32(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_int32


!-------------------------------------------------------------------------------

!> Get a variable from the ObsSapce database

subroutine obsspace_get_db_int64(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int64_t), intent(inout) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_get_int64(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_int64

!-------------------------------------------------------------------------------

!> Get a variable from the ObsSapce database

subroutine obsspace_get_db_real32(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_float), intent(inout) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_get_real32(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_real32

!-------------------------------------------------------------------------------

!> Get a variable from the ObsSapce database

subroutine obsspace_get_db_real64(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_double), intent(inout) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_get_real64(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_real64

!-------------------------------------------------------------------------------

!> Get datetime from the ObsSapce database

subroutine obsspace_get_db_datetime(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  type(datetime), intent(inout) :: vect(:)

  integer(c_size_t) :: length, i
  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_int32_t), dimension(:), allocatable :: date
  integer(c_int32_t), dimension(:), allocatable :: time
  character(len=20) :: fstring

  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  allocate(date(length), time(length))
  call c_obsspace_get_datetime(obss, c_group, c_vname, length, date, time)

  ! Constrct datatime based on date and time
  do i = 1, length
    write(fstring, "(i4.4, a, i2.2, a, i2.2, a, i2.2, a, i2.2, a, i2.2, a)") &
          date(i)/10000, '-', MOD(date(i), 10000)/100, '-', MOD(MOD(date(i), 10000), 100), 'T', &
          time(i)/10000, ':', MOD(time(i), 10000)/100, ':', MOD(MOD(time(i), 10000), 100), 'Z'
    call datetime_create(fstring, vect(i))
  enddo

  ! Clean up
  deallocate(date, time)
end subroutine obsspace_get_db_datetime

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_int32(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int32_t), intent(in) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_put_int32(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_int32

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_int64(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int64_t), intent(in) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_put_int64(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_int64

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_real32(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_float), intent(in) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_put_real32(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_real32

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_real64(obss, group, vname, vect)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_double), intent(in) :: vect(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  call c_obsspace_put_real64(obss, c_group, c_vname, length, vect)

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_real64

!-------------------------------------------------------------------------------

end module obsspace_mod
