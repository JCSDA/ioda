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
public obsspace_get_var

#include "obsspace_interface.f"

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

!> Get a metadata variable

subroutine obsspace_get_var(c_dom, vdata, vname, vsize)
     implicit none

     type(c_ptr), intent(in) :: c_dom
     real(kind=c_double) :: vdata(vsize)
     character(len=*), intent(in) :: vname
     integer(kind=c_int), value :: vsize

     character(kind=c_char, len=1), allocatable :: c_vname(:)

     ! Convert fortran string to c string (null terminated)
     call f_c_string(vname, c_vname)

     ! Call the interface to access the getvar method in ObsSpace
     call c_obsspace_get_var(c_dom, c_vname, vsize, vdata)

     ! Clean up - the f_c_string routine allocated c_vname
     deallocate(c_vname)
end subroutine obsspace_get_var

!-------------------------------------------------------------------------------

end module obsspace_mod
