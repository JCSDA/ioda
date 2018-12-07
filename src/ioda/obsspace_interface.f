!
! (C) Copyright 2017 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Define interface for C++ ObsSpace code called from Fortran

!-------------------------------------------------------------------------------
interface
!-------------------------------------------------------------------------------

!use datetime_mod

integer(kind=c_int) function c_obsspace_get_nobs(dom) bind(C,name='obsspace_get_nobs_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: dom
end function c_obsspace_get_nobs

integer(kind=c_int) function c_obsspace_get_nlocs(dom) bind(C,name='obsspace_get_nlocs_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: dom
end function c_obsspace_get_nlocs

logical(kind=c_bool) function c_obsspace_has(dom, group, vname) bind(C,name='obsspace_has_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value                        :: dom
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
end function c_obsspace_has

real(kind=c_double) function c_obspace_missing_value() bind(C,name='obspace_missing_value_f')
  use, intrinsic :: iso_c_binding
  implicit none
end function c_obspace_missing_value

!-------------------------------------------------------------------------------
! get data from ObsSpace

subroutine c_obsspace_get_refdate(obss, c_refdate) &
              & bind(C,name='obsspace_get_refdate_f')
  use, intrinsic :: iso_c_binding, only : c_ptr
  implicit none
  type(c_ptr), value :: obss
  type(c_ptr), intent(out) :: c_refdate
end subroutine c_obsspace_get_refdate

subroutine c_obsspace_get_int32(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_get_int32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int32_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int32_t), intent(inout) :: vect(length)
end subroutine c_obsspace_get_int32
  
subroutine c_obsspace_get_int64(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_get_int64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int64_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int64_t), intent(inout) :: vect(length)
end subroutine c_obsspace_get_int64
  
subroutine c_obsspace_get_real32(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_get_real32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_float
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_float), intent(inout) :: vect(length)
end subroutine c_obsspace_get_real32
  
subroutine c_obsspace_get_real64(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_get_real64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_double
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_double), intent(inout) :: vect(length)
end subroutine c_obsspace_get_real64
  
!-------------------------------------------------------------------------------
! store data in ObsSpace

subroutine c_obsspace_put_int32(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_put_int32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int32_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int32_t), intent(in) :: vect(length)
end subroutine c_obsspace_put_int32

subroutine c_obsspace_put_int64(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_put_int64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int64_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int64_t), intent(in) :: vect(length)
end subroutine c_obsspace_put_int64

subroutine c_obsspace_put_real32(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_put_real32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_float
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_float), intent(in) :: vect(length)
end subroutine c_obsspace_put_real32

subroutine c_obsspace_put_real64(obss, group, vname, length, vect) &
              & bind(C,name='obsspace_put_real64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_double
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_double), intent(in) :: vect(length)
end subroutine c_obsspace_put_real64

!-------------------------------------------------------------------------------
end interface
!-------------------------------------------------------------------------------

