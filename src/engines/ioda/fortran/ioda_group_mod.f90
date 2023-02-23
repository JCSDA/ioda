module ioda_group_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env
   use :: ioda_vecstring_mod
   use :: ioda_f_c_string_mod

   type :: ioda_group
      type(c_ptr) :: data_ptr
   contains
      final ioda_group_dtor
      procedure :: list => ioda_group_list
      procedure :: open => ioda_group_open
      procedure :: create => ioda_group_create
      procedure :: exists => ioda_group_exists

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

   subroutine ioda_group_list(this, vstr)
      implicit none
      class(ioda_group) :: this
      class(ioda_vecstring) :: vstr
      if (c_associated(vstr%data_ptr)) then
         call ioda_vecstring_dealloc(vstr)
      end if
      vstr%data_ptr = ioda_group_c_list(this%data_ptr)
   end subroutine

   function ioda_group_exists(this, sz, name) result(r)
      implicit none
      class(ioda_group) :: this
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_str
      integer(int64) ::  sz
      integer(int32) :: r

      name_str = ioda_f_string_to_c_dup(name)
      r = ioda_group_c_exists(this%data_ptr, sz, name_str)
      call ioda_c_free(name_str)
   end function

   subroutine ioda_group_create(this, sz, name, new_grp)
      implicit none
      class(ioda_group) :: this
      class(ioda_group) :: new_grp
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_str
      integer(int64) ::  sz

      if (c_associated(new_grp%data_ptr)) then
         call ioda_group_dtor(new_grp)
      end if
      name_str = ioda_f_string_to_c_dup(name)
      new_grp%data_ptr = ioda_group_c_create(this%data_ptr, sz, name_str)
      call ioda_c_free(name_str)
   end subroutine

   subroutine ioda_group_open(this, sz, name, new_grp)
      implicit none
      class(ioda_group) :: this
      class(ioda_group) :: new_grp
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_str
      integer(int64) ::  sz

      if (c_associated(new_grp%data_ptr)) then
         call ioda_group_dtor(new_grp)
      end if
      sz = len_trim(name)
      name_str = ioda_f_string_to_c_dup(name)
      new_grp%data_ptr = ioda_group_c_open(this%data_ptr, sz, name_str)
      call ioda_c_free(name_str)
   end subroutine

end module
