!
! (C) Copyright 2023 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module ioda_group_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env
   use :: cxx_vector_string_mod
   use :: f_c_string_mod
   use :: ioda_has_attributes_mod
   use :: ioda_has_variables_mod

   type :: ioda_group
      type(c_ptr) :: data_ptr  = c_null_ptr
   contains
      final ioda_group_dtor
      procedure :: list => ioda_group_list
      procedure :: open => ioda_group_open
      procedure :: create => ioda_group_create
      procedure :: exists => ioda_group_exists
      procedure :: has_attributes => ioda_group_has_attributes
      procedure :: has_variables => ioda_group_has_variables

      procedure, private, pass(this) :: ioda_group_copy
      generic, public :: assignment(=) => ioda_group_copy

   end type
   interface
      function ioda_group_c_alloc() result(p) bind(C, name="ioda_group_c_alloc")
         import c_ptr
         type(C_ptr) :: p
      end function

      subroutine ioda_group_c_dtor(p) bind(C, name="ioda_group_c_dtor")
         import c_ptr
         type(C_ptr) :: p
      end subroutine

      subroutine ioda_group_c_clone(this, rhs) bind(C, name="ioda_group_c_clone")
         import c_ptr
         type(c_ptr), value :: rhs
         type(c_ptr) :: this
      end subroutine

      function ioda_group_c_list(p) result(vstr) bind(C, name="ioda_group_c_list")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr) :: vstr
      end function

      function ioda_group_c_exists(p, sz, name) result(r) bind(C, name="ioda_group_c_exists")
         import c_ptr, c_int, c_int64_t
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         integer(c_int64_t) ::  sz
         integer(c_int) :: r
      end function

      function ioda_group_c_create(p, sz, name) result(new_grp) bind(C, name="ioda_group_c_create")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(C_ptr) :: new_grp
         integer(c_int64_t) :: sz
      end function

      function ioda_group_c_open(p, sz, name) result(new_grp) bind(C, name="ioda_group_c_open")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(C_ptr) :: new_grp
         integer(c_int64_t) :: sz
      end function

      function ioda_group_c_has_attributes(p) result(has_p) bind(C, name="ioda_group_c_has_attributes")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr) :: has_p
      end function

      function ioda_group_c_has_variables(p) result(has_p) bind(C, name="ioda_group_c_has_variables")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr) :: has_p
      end function

   end interface
contains
   subroutine ioda_group_init(this)
      implicit none
      type(ioda_group) :: this
      this%data_ptr = ioda_group_c_alloc()
   end subroutine

   subroutine ioda_group_dtor(this)
      implicit none
      type(ioda_group) :: this
      call ioda_group_c_dtor(this%data_ptr)
   end subroutine

   subroutine ioda_group_copy(this, rhs)
      implicit none
      class(ioda_group), intent(in) :: rhs
      class(ioda_group), intent(out) :: this
      call ioda_group_c_clone(this%data_ptr, rhs%data_ptr)
   end subroutine

   function ioda_group_list(this) result(vstr)
      implicit none
      class(ioda_group) :: this
      type(cxx_vector_string) :: vstr
      vstr%data_ptr = ioda_group_c_list(this%data_ptr)
   end function

   function ioda_group_exists(this, sz, name) result(r)
      implicit none
      class(ioda_group) :: this
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_str
      integer(int64), intent(in) ::  sz
      integer(int32) :: r

      name_str = f_string_to_c_dup(name)
      r = ioda_group_c_exists(this%data_ptr, sz, name_str)
      call c_free(name_str)
   end function

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
   end function

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
   end function

   function ioda_group_has_attributes(this) result(has_att)
      implicit none
      class(ioda_group), intent(in) :: this
      type(ioda_has_attributes) :: has_att
      has_att%data_ptr = ioda_group_c_has_attributes(this%data_ptr)
   end function

   function ioda_group_has_variables(this) result(has_var)
      implicit none
      class(ioda_group), intent(in) :: this
      type(ioda_has_variables) :: has_var
      has_var%data_ptr = ioda_group_c_has_variables(this%data_ptr)
   end function

end module
