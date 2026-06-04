!
! (C) Copyright 2023 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module ioda_group_mod
   use, intrinsic :: iso_c_binding, only: c_int, c_int64_t, c_null_ptr, c_ptr
   use, intrinsic :: iso_fortran_env, only: int32, int64
   use :: cxx_vector_string_mod, only: cxx_vector_string
   use :: f_c_string_mod, only: c_free, f_string_to_c_dup
   use :: ioda_has_attributes_mod, only: ioda_has_attributes
   use :: ioda_has_variables_mod, only: ioda_has_variables
   implicit none
   private

   type, public :: ioda_group
      type(c_ptr) :: data_ptr  = c_null_ptr
   contains
      final :: ioda_group_dtor
      procedure :: list => ioda_group_list
      procedure :: open => ioda_group_open
      procedure :: create => ioda_group_create
      procedure :: exists => ioda_group_exists
      procedure :: has_attributes => ioda_group_has_attributes
      procedure :: has_variables => ioda_group_has_variables

      procedure, private, pass(this) :: ioda_group_copy
      generic, public :: assignment(=) => ioda_group_copy

   end type ioda_group
   interface
      function ioda_group_c_alloc() result(p) bind(C, name="ioda_group_c_alloc")
         import c_ptr
         implicit none
         type(c_ptr) :: p
      end function ioda_group_c_alloc

      subroutine ioda_group_c_dtor(p) bind(C, name="ioda_group_c_dtor")
         import c_ptr
         implicit none
         type(c_ptr), intent(inout) :: p
      end subroutine ioda_group_c_dtor

      subroutine ioda_group_c_clone(this, rhs) bind(C, name="ioda_group_c_clone")
         import c_ptr
         implicit none
         type(c_ptr), value :: rhs
         type(c_ptr), intent(inout) :: this
      end subroutine ioda_group_c_clone

      function ioda_group_c_list(p) result(vstr) bind(C, name="ioda_group_c_list")
         import c_ptr
         implicit none
         type(c_ptr), value :: p
         type(c_ptr) :: vstr
      end function ioda_group_c_list

      function ioda_group_c_exists(p, sz, name) result(r) bind(C, name="ioda_group_c_exists")
         import c_ptr, c_int, c_int64_t
         implicit none
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) ::  sz
         integer(c_int) :: r
      end function ioda_group_c_exists

      function ioda_group_c_create(p, sz, name) result(new_grp) bind(C, name="ioda_group_c_create")
         import c_ptr, c_int64_t
         implicit none
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr) :: new_grp
         integer(c_int64_t), intent(in) :: sz
      end function ioda_group_c_create

      function ioda_group_c_open(p, sz, name) result(new_grp) bind(C, name="ioda_group_c_open")
         import c_ptr, c_int64_t
         implicit none
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr) :: new_grp
         integer(c_int64_t), intent(in) :: sz
      end function ioda_group_c_open

      function ioda_group_c_has_attributes(p) result(has_p) bind(C, name="ioda_group_c_has_attributes")
         import c_ptr
         implicit none
         type(c_ptr), value :: p
         type(c_ptr) :: has_p
      end function ioda_group_c_has_attributes

      function ioda_group_c_has_variables(p) result(has_p) bind(C, name="ioda_group_c_has_variables")
         import c_ptr
         implicit none
         type(c_ptr), value :: p
         type(c_ptr) :: has_p
      end function ioda_group_c_has_variables

   end interface
contains
   subroutine ioda_group_init(this)
      implicit none
      type(ioda_group), intent(inout) :: this
      this%data_ptr = ioda_group_c_alloc()
   end subroutine ioda_group_init

   subroutine ioda_group_dtor(this)
      implicit none
      type(ioda_group), intent(inout) :: this
      call ioda_group_c_dtor(this%data_ptr)
   end subroutine ioda_group_dtor

   subroutine ioda_group_copy(this, rhs)
      implicit none
      class(ioda_group), intent(in) :: rhs
      class(ioda_group), intent(out) :: this
      call ioda_group_c_clone(this%data_ptr, rhs%data_ptr)
   end subroutine ioda_group_copy

   function ioda_group_list(this) result(vstr)
      implicit none
      class(ioda_group), intent(in) :: this
      type(cxx_vector_string) :: vstr
      vstr%data_ptr = ioda_group_c_list(this%data_ptr)
   end function ioda_group_list

   function ioda_group_exists(this, sz, name) result(r)
      implicit none
      class(ioda_group), intent(in) :: this
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_str
      integer(int64), intent(in) ::  sz
      integer(int32) :: r

      name_str = f_string_to_c_dup(name)
      r = ioda_group_c_exists(this%data_ptr, sz, name_str)
      call c_free(name_str)
   end function ioda_group_exists

   function ioda_group_create(this, sz, name) result(new_grp)
      implicit none
      class(ioda_group), intent(in) :: this
      type(ioda_group) :: new_grp
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_str
      integer(int64), intent(in) ::  sz

      name_str = f_string_to_c_dup(name)
      new_grp%data_ptr = ioda_group_c_create(this%data_ptr, sz, name_str)
      call c_free(name_str)
   end function ioda_group_create

   function ioda_group_open(this, sz, name) result(new_grp)
      implicit none
      class(ioda_group), intent(in) :: this
      type(ioda_group) :: new_grp
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_str
      integer(int64), intent(in) ::  sz

      name_str = f_string_to_c_dup(name)
      new_grp%data_ptr = ioda_group_c_open(this%data_ptr, sz, name_str)
      call c_free(name_str)
   end function ioda_group_open

   function ioda_group_has_attributes(this) result(has_att)
      implicit none
      class(ioda_group), intent(in) :: this
      type(ioda_has_attributes) :: has_att
      has_att%data_ptr = ioda_group_c_has_attributes(this%data_ptr)
   end function ioda_group_has_attributes

   function ioda_group_has_variables(this) result(has_var)
      implicit none
      class(ioda_group), intent(in) :: this
      type(ioda_has_variables) :: has_var
      has_var%data_ptr = ioda_group_c_has_variables(this%data_ptr)
   end function ioda_group_has_variables

end module ioda_group_mod
