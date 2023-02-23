module ioda_vecstring_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env
   use :: ioda_f_c_string_mod

   type :: ioda_string
      type(c_ptr) :: data_ptr  ! handle to underlying ioda_string_t
   contains
      final ioda_string_dealloc
      procedure :: get => ioda_string_get
      procedure :: append => ioda_string_append
      procedure :: set => ioda_string_set
      procedure :: append_string => ioda_string_append_string
      procedure :: set_string => ioda_string_set_string
      procedure :: clear => ioda_string_clear
      procedure :: size => ioda_string_size
      procedure, private, pass(this) ::ioda_string_copy
      generic, public :: assignment(=) => ioda_string_copy
   end type

   type :: ioda_vecstring
      type(c_ptr) :: data_ptr ! handle to underlying ioda_vecstring_t
   contains
      final ioda_vecstring_dealloc
      procedure :: get => ioda_vecstring_get
      procedure :: append => ioda_vecstring_append
      procedure :: set => ioda_vecstring_set
      procedure :: get_string => ioda_vecstring_get_string
      procedure :: append_string => ioda_vecstring_append_string
      procedure :: set_string => ioda_vecstring_set_string
      procedure :: copy => ioda_vecstring_copy
      procedure :: clear => ioda_vecstring_clear
      procedure :: size => ioda_vecstring_size
      procedure :: resize => ioda_vecstring_resize
      procedure :: element_size => ioda_vecstring_element_size
      procedure :: push_back => ioda_vecstring_push_back
      procedure :: push_back_string => ioda_vecstring_push_back_string
      procedure, private, pass(this) ::ioda_vecstring_copy
      generic, public :: assignment(=) => ioda_vecstring_copy

   end type

   interface
      function ioda_vecstring_c_alloc() result(p) bind(C, name="ioda_vecstring_c_alloc")
         import c_ptr
         type(c_ptr) :: p
      end function

      subroutine ioda_vecstring_c_dealloc(p) bind(C, name="ioda_vecstring_c_dealloc")
         import c_ptr
         type(c_ptr), value :: p
      end subroutine

      subroutine ioda_vecstring_c_clone(this, rhs) bind(C, name="ioda_vecstring_c_clone")
         import c_ptr
         type(c_ptr), value :: rhs
         type(c_ptr) :: this
      end subroutine

      subroutine ioda_vecstring_c_set_string(p, i, str) bind(C, name="ioda_vecstring_c_set_string_f")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: i
         type(c_ptr), value :: str
      end subroutine

      subroutine ioda_vecstring_c_append_string(p, i, str) bind(C, name="ioda_vecstring_c_append_string_f")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: i
         type(c_ptr), value :: str
      end subroutine

      function ioda_vecstring_c_get_string(p, i) result(str) bind(C, name="ioda_vecstring_c_get_string_f")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: i
         type(c_ptr) :: str
      end function

      subroutine ioda_vecstring_c_set(p, i, str) bind(C, name="ioda_vecstring_c_set_f")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: i
         type(c_ptr), value :: str
      end subroutine

      subroutine ioda_vecstring_c_append(p, i, str) bind(C, name="ioda_vecstring_c_append_f")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: i
         type(c_ptr), value :: str
      end subroutine

      function ioda_vecstring_c_get(p, i) result(str) bind(C, name="ioda_vecstring_c_get_f")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: i
         type(c_ptr) :: str
      end function

      function ioda_vecstring_c_copy(p) result(pnew) bind(C, name="ioda_vecstring_c_copy")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr) :: pnew
      end function

      subroutine ioda_vecstring_c_clear(p) bind(C, name="ioda_vecstring_c_clear")
         import c_ptr
         type(c_ptr), value :: p
      end subroutine

      subroutine ioda_vecstring_c_resize(p, n) bind(C, name="ioda_vecstring_c_resize")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: n
      end subroutine

      function ioda_vecstring_c_size(p) result(sz) bind(C, name="ioda_vecstring_c_size")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz
      end function

      function ioda_vecstring_c_element_size(p, i) result(sz) bind(C, name="ioda_vecstring_c_element_size_f")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), value :: i
         integer(c_int64_t) :: sz
      end function

      subroutine ioda_vecstring_c_push_back(p, s) bind(C, name="ioda_vecstring_c_push_back")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr), value :: s
      end subroutine

      subroutine ioda_vecstring_c_push_back_string(p, s) bind(C, name="ioda_vecstring_c_push_back_string")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr), value :: s
      end subroutine

      function ioda_string_c_alloc() result(p) bind(C, name="ioda_string_c_alloc")
         import c_ptr
         type(c_ptr) :: p
      end function

      subroutine ioda_string_c_dealloc(p) bind(C, name="ioda_string_c_dealloc")
         import c_ptr
         type(c_ptr), value :: p
      end subroutine

      subroutine ioda_string_c_clone(this, rhs) bind(C, name="ioda_string_c_clone")
         import c_ptr
         type(c_ptr), value :: rhs
         type(c_ptr) :: this
      end subroutine

      subroutine ioda_string_c_set_string(p, str) bind(C, name="ioda_string_c_set_string")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr), value :: str
      end subroutine

      subroutine ioda_string_c_append_string(p, str) bind(C, name="ioda_string_c_append_string")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr), value :: str
      end subroutine

      function ioda_string_c_get(p) result(s) bind(C, name="ioda_string_c_get")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr) :: s
      end function

      subroutine ioda_string_c_set(p, str) bind(C, name="ioda_string_c_set")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr), value :: str
      end subroutine

      subroutine ioda_string_c_append(p, str) bind(C, name="ioda_string_c_append")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr), value :: str
      end subroutine

      subroutine ioda_string_c_clear(p) bind(C, name="ioda_string_c_clear")
         import c_ptr
         type(c_ptr), value :: p
      end subroutine

      function ioda_string_c_size(p) result(sz) bind(C, name="ioda_string_c_size")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz
      end function

      function ioda_string_c_copy(p, pnew) bind(C, name="ioda_string_c_copy")
         import c_ptr
         type(C_ptr), value :: p
         type(C_ptr), value :: pnew
      end function
   end interface

contains
   subroutine ioda_vecstring_init(v)
      implicit none
      type(ioda_vecstring) :: v
      v%data_ptr = ioda_vecstring_c_alloc()
   end subroutine

   subroutine ioda_vecstring_copy(this, rhs)
      implicit none
      class(ioda_vecstring), intent(in) :: rhs
      class(ioda_vecstring), intent(out) :: this
      call ioda_vecstring_c_clone(this%data_ptr, rhs%data_ptr)
   end subroutine

   subroutine ioda_vecstring_dealloc(this)
      implicit none
      type(ioda_vecstring) :: this
      call ioda_vecstring_c_dealloc(this%data_ptr)
   end subroutine

   subroutine ioda_vecstring_set(this, i, fstr)
      implicit none
      class(ioda_vecstring) :: this
      integer(int64), intent(in) :: i
      character(len=*), intent(in) :: fstr
      type(c_ptr) :: str_ptr

      str_ptr = ioda_f_string_to_c_dup(fstr)
      call ioda_vecstring_c_set(this%data_ptr, i, str_ptr)
      call c_free(str_ptr)
   end subroutine

   subroutine ioda_vecstring_append(this, i, fstr)
      implicit none
      class(ioda_vecstring) :: this
      integer(int64), intent(in) :: i
      character(len=*), intent(in) :: fstr
      type(c_ptr) :: str_ptr

      str_ptr = ioda_f_string_to_c_dup(fstr)
      call ioda_vecstring_c_append(this%data_ptr, i, str_ptr)
      call c_free(str_ptr)
   end subroutine

   subroutine ioda_vecstring_get(this, i, fstr)
      implicit none
      class(ioda_vecstring) :: this
      integer(int64), intent(in) :: i
      character(len=*), intent(out) :: fstr
      type(c_ptr) :: str_ptr

      str_ptr = ioda_vecstring_c_get(this%data_ptr, i)
      call ioda_c_string_to_f_copy(str_ptr, fstr)
      call c_free(str_ptr)
   end subroutine

   subroutine ioda_vecstring_get_string(this, i, str)
      implicit none
      class(ioda_vecstring) :: this
      class(ioda_string), intent(out) :: str
      integer(int64), intent(in) :: i
      type(c_ptr) :: rs

      rs = ioda_vecstring_c_get_string(this%data_ptr, i)
      call ioda_string_c_set_string(str%data_ptr, rs)
   end subroutine

   subroutine ioda_vecstring_set_string(this, i, str)
      implicit none
      class(ioda_vecstring) :: this
      class(ioda_string), intent(in) :: str
      integer(int64), intent(in) :: i
      call ioda_vecstring_c_set_string(this%data_ptr, i, str%data_ptr)
   end subroutine

   subroutine ioda_vecstring_append_string(this, i, str)
      implicit none
      class(ioda_vecstring) :: this
      class(ioda_string), intent(in) :: str
      integer(int64), intent(in) :: i
      call ioda_vecstring_c_append_string(this%data_ptr, i, str%data_ptr)
   end subroutine

   subroutine ioda_vecstring_clear(this)
      implicit none
      class(ioda_vecstring) :: this
      call ioda_vecstring_c_clear(this%data_ptr)
   end subroutine

   subroutine ioda_vecstring_resize(this, n)
      implicit none
      class(ioda_vecstring) :: this
      integer(int64) :: n
      call ioda_vecstring_c_resize(this%data_ptr, n)
   end subroutine

   subroutine ioda_vecstring_push_back(this, fstr)
      implicit none
      class(ioda_vecstring) :: this
      character(len=*), intent(in) :: fstr
      type(c_ptr) :: str_ptr

      str_ptr = ioda_f_string_to_c_dup(fstr)
      call ioda_vecstring_c_push_back(this%data_ptr, str_ptr)
      call c_free(str_ptr)
   end subroutine

   subroutine ioda_vecstring_push_back_string(this, str)
      implicit none
      class(ioda_vecstring) :: this
      class(ioda_string), intent(in) :: str

      call ioda_vecstring_c_push_back_string(this%data_ptr, str%data_ptr)
   end subroutine

   integer(int64) function ioda_vecstring_size(this) result(sz)
      implicit none
      class(ioda_vecstring) :: this
      sz = ioda_vecstring_c_size(this%data_ptr)
   end function

   integer(int64) function ioda_vecstring_element_size(this, i) result(sz)
      implicit none
      class(ioda_vecstring) :: this
      integer(int64), intent(in) :: i
      sz = ioda_vecstring_c_element_size(this%data_ptr, i)
   end function

   subroutine ioda_string_init(v)
      implicit none
      type(ioda_string) :: v
      v%data_ptr = ioda_string_c_alloc()
   end subroutine

   subroutine ioda_string_dealloc(this)
      implicit none
      type(ioda_string) :: this
      call ioda_string_c_dealloc(this%data_ptr)
   end subroutine

   subroutine ioda_string_copy(this, rhs)
      implicit none
      class(ioda_string), intent(in) :: rhs
      class(ioda_string), intent(out) :: this
      call ioda_string_c_clone(this%data_ptr, rhs%data_ptr)
   end subroutine

   subroutine ioda_string_set(this, fstr)
      implicit none
      class(ioda_string) :: this
      character(len=*), intent(in) :: fstr
      type(c_ptr) :: str_ptr

      str_ptr = ioda_f_string_to_c_dup(fstr)
      call ioda_string_c_set(this%data_ptr, str_ptr)
      call c_free(str_ptr)
   end subroutine

   subroutine ioda_string_append(this, fstr)
      implicit none
      class(ioda_string) :: this
      character(len=*), intent(in) :: fstr
      type(c_ptr) :: str_ptr

      str_ptr = ioda_f_string_to_c_dup(fstr)
      call ioda_string_c_append(this%data_ptr, str_ptr)
      call c_free(str_ptr)
   end subroutine

   subroutine ioda_string_get(this, fstr)
      implicit none
      class(ioda_string) :: this
      character(len=*), intent(out) :: fstr
      type(c_ptr) :: str_ptr

      str_ptr = ioda_string_c_get(this%data_ptr)
      call ioda_c_string_to_f_copy(str_ptr, fstr)
      call c_free(str_ptr)
   end subroutine

   subroutine ioda_string_set_string(this, str)
      implicit none
      class(ioda_string) :: this
      class(ioda_string), intent(in) :: str
      call ioda_string_c_set_string(this%data_ptr, str%data_ptr)
   end subroutine

   subroutine ioda_string_append_string(this, str)
      implicit none
      class(ioda_string) :: this
      class(ioda_string), intent(in) :: str
      call ioda_string_c_append_string(this%data_ptr, str%data_ptr)
   end subroutine

   subroutine ioda_string_clear(this)
      implicit none
      class(ioda_string) :: this
      call ioda_string_c_clear(this%data_ptr)
   end subroutine

   integer(int64) function ioda_string_size(this) result(sz)
      implicit none
      class(ioda_string), intent(in) :: this
      sz = ioda_string_c_size(this%data_ptr)
   end function

   subroutine ioda_command_line_to_vecstring(vstring)
      implicit none
      class(ioda_vecstring), intent(out) :: vstring
      integer :: argc
      character(len=512) :: argv
      integer :: i

      argc = command_argument_count()
      write (error_unit, *) 'fargc = ', argc
      do i = 1, argc
         call get_command_argument(i, argv)
         write (error_unit, *) i, ' ', trim(argv), ' ', len_trim(argv)
      end do
      if (.not. c_associated(vstring%data_ptr)) then
         call ioda_vecstring_init(vstring)
      end if
      write (error_unit, *) 'R1 clearing vecstring'
      call ioda_vecstring_clear(vstring)
      write (error_unit, *) 'r2 get count'
      argc = command_argument_count()
      write (error_unit, *) 'argf =', argc
      do i = 1, argc
         call get_command_argument(i, argv)
         write (error_unit, *) i, ' ', trim(argv)
         call ioda_vecstring_push_back(vstring, argv)
      end do
      write (error_unit, *) 'r4 end loop'
      argc = ioda_vecstring_size(vstring)
      write (error_unit, *) 'vecstring size = ', argc
   end subroutine

end module

