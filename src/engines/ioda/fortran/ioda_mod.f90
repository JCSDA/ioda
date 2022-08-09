!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  @file ioda_mod.f90
!>  @brief Fortran interface to ioda. Header-like definitions only.

!>  @brief Fortran interface to ioda. Header-like definitions only..
module ioda_f
    use, intrinsic :: iso_c_binding
    implicit none
  
    public
  
    type, bind(C) :: ioda_c_interface_t
      type(c_ptr) :: Engines
      type(c_ptr) :: Groups
      type(c_ptr) :: Strings
      type(c_ptr) :: VecStrings
    end type ioda_c_interface_t
  
  
    interface
  
    type(c_ptr) function get_ioda_c_interface() bind(C, name='get_ioda_c_interface')
      use, intrinsic :: iso_c_binding
      import
      implicit none
    end function get_ioda_c_interface
  
    end interface
  
    contains
  
  
end module ioda_f
  
