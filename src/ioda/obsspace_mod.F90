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

implicit none

private
public obsspace_get_nobs
public obsspace_get_nlocs
public obsspace_get_db
public obsspace_put_db
public obspace_missing_value

#include "obsspace_interface.f"

!-------------------------------------------------------------------------------

interface obsspace_get_db
  module procedure obsspace_get_db_int32
  module procedure obsspace_get_db_int64
  module procedure obsspace_get_db_real32
  module procedure obsspace_get_db_real64
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

!>  Return the number of observations

integer function obsspace_get_nobs(c_dom)
  implicit none
  type(c_ptr), intent(in) :: c_dom

  obsspace_get_nobs = c_obsspace_get_nobs(c_dom)
end function obsspace_get_nobs

!-------------------------------------------------------------------------------

!>  Return the number of observational locations

integer function obsspace_get_nlocs(c_dom)
  implicit none
  type(c_ptr), intent(in) :: c_dom

  obsspace_get_nlocs = c_obsspace_get_nlocs(c_dom)
end function obsspace_get_nlocs

!-------------------------------------------------------------------------------

!>  Return the missing value indicator

real(c_double) function obspace_missing_value()
  implicit none
  obspace_missing_value = c_obspace_missing_value()
end function obspace_missing_value

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
