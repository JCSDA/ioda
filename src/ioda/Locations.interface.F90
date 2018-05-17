!
!  (C) Copyright 2017 UCAR
!  
!  This software is licensed under the terms of the Apache Licence Version 2.0
!  which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
!

module ioda_locs_mod_c

use iso_c_binding
use ioda_locs_mod
use kinds

implicit none

public :: ioda_locs_registry

private

! ------------------------------------------------------------------------------

#define LISTED_TYPE ioda_locs

!> Linked list interface - defines registry_t type
#include "linkedList_i.f"

!> Global registry
type(registry_t) :: ioda_locs_registry

! ------------------------------------------------------------------------------
contains
! ------------------------------------------------------------------------------
!> Linked list implementation
#include "linkedList_c.f"

! ------------------------------------------------------------------------------

subroutine ioda_locs_create_c(key, klocs, klats, klons) bind(c,name='ioda_locs_create_f90')

implicit none
integer(c_int), intent(inout) :: key
integer(c_int), intent(in) :: klocs
real(c_double), intent(in) :: klats(klocs)
real(c_double), intent(in) :: klons(klocs)

type(ioda_locs), pointer :: self
real(kind_real) :: lats(klocs)
real(kind_real) :: lons(klocs)

call ioda_locs_registry%init()
call ioda_locs_registry%add(key)
call ioda_locs_registry%get(key, self)

lats(:) = klats(:)
lons(:) = klons(:)

call ioda_locs_create(self, klocs, lats, lons)

end subroutine ioda_locs_create_c

! ------------------------------------------------------------------------------

subroutine ioda_locs_delete_c(key) bind(c,name='ioda_locs_delete_f90')

implicit none
integer(c_int), intent(inout) :: key
type(ioda_locs), pointer :: self

call ioda_locs_registry%get(key,self)
call ioda_locs_delete(self)
call ioda_locs_registry%remove(key)

end subroutine ioda_locs_delete_c

! ------------------------------------------------------------------------------

subroutine ioda_locs_nobs_c(key, kobs) bind(c,name='ioda_locs_nobs_f90')

implicit none
integer(c_int), intent(in) :: key
integer(c_int), intent(inout) :: kobs
type(ioda_locs), pointer :: self

call ioda_locs_registry%get(key,self)
kobs = self%nlocs

end subroutine ioda_locs_nobs_c
! ------------------------------------------------------------------------------

subroutine ioda_locs_coords_c(key,idx,mylat,mylon) bind(c,name='ioda_locs_coords_f90')

implicit none
integer(c_int), intent(in) :: key
integer(c_int), intent(in) :: idx
real(c_double), intent(inout) :: mylat,mylon

type(ufo_locs), pointer :: self

call ufo_locs_registry%get(key,self)
mylat = self%lat(idx)
mylon = self%lon(idx)

end subroutine ioda_locs_coords_c

! ------------------------------------------------------------------------------

end module ioda_locs_mod_c
