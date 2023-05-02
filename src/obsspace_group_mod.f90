!
! (C) Copyright 2017 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module obsspace_group_mod
   use, intrinsic :: iso_c_binding
   use :: ioda_group_mod

   interface
      function c_ioda_obs_space_get_group(ptr) result(grp_ptr) &
         & bind(C, name="c_ioda_obs_space_get_group")
         import c_ptr
         type(c_ptr), value :: ptr
         type(c_ptr) :: grp_ptr
      end function
   end interface

contains

   function ioda_obs_space_get_group(obs_space_ptr) result(grp)
      implicit none
      type(c_ptr) :: obs_space_ptr
      type(ioda_group) :: grp
      grp%data_ptr = c_ioda_obs_space_get_group(obs_space_ptr)
   end function

end module
