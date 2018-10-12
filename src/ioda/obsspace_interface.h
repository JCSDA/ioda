!
! (C) Copyright 2017 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Define interface for C++ ObsSpace code called from Fortran

!-------------------------------------------------------------------------------
interface
!-------------------------------------------------------------------------------

integer(kind=c_int) function c_obsspace_get_nobs(dom) bind(C,name='obsspace_get_nobs_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: dom
end function c_obsspace_get_nobs

subroutine c_obsspace_get_mdata(dom, vname, vdata, vsize) &
         bind(C,name='obsspace_get_mdata_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr) :: result_ptr

  type(c_ptr), value :: dom
  character(kind=c_char,len=1), dimension(*) :: vname
  real(kind=c_double) :: vdata(vsize)
  integer(kind=c_int), value :: vsize
end subroutine c_obsspace_get_mdata

!-------------------------------------------------------------------------------
end interface
!-------------------------------------------------------------------------------
