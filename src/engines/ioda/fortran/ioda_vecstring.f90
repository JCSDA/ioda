!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  @file ioda_vecstring.f90
!>  @brief Fortran interface to ioda VecStrings. Used to encapsulate C++ vector<string>.

!>  @brief Fortran interface to ioda VecStrings. Used to encapsulate C++ vector<string>.
module ioda_vecstring_f
    use, intrinsic :: iso_c_binding
    use :: ioda_f !
    implicit none
  
    public
  
    type, bind(C) :: ioda_vecstring_c
      type(c_funptr) :: destruct
      type(c_funptr) :: construct
      type(c_funptr) :: copy
      type(c_funptr) :: clear
      type(c_funptr) :: getAsCharArray
      type(c_funptr) :: getAsCharArray2
      type(c_funptr) :: setFromCharArray
      type(c_funptr) :: elementSize
      type(c_funptr) :: size
      type(c_funptr) :: resize
      type(c_ptr)    :: data_
    end type ioda_vecstring_c
  
    abstract interface
      subroutine ioda_vecstring_c_destruct_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end subroutine ioda_vecstring_c_destruct_f

      type(c_ptr) function ioda_vecstring_c_construct_f() bind(C)
        use, intrinsic :: iso_c_binding
      end function ioda_vecstring_c_construct_f
  
      type(c_ptr) function ioda_vecstring_c_copy_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end function ioda_vecstring_c_copy_f
  
      subroutine ioda_vecstring_c_clear_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end subroutine ioda_vecstring_c_clear_f
  
      integer(c_size_t) function ioda_vecstring_c_get_f(obj, n, buf, buf_len) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: n
        character(kind=c_char,len=1), intent(out) :: buf(*)
        integer(c_size_t), value :: buf_len
      end function ioda_vecstring_c_get_f

      integer(c_size_t) function ioda_vecstring_c_get2_f(obj, n, buf, buf_len, empty_char) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: n
        character(kind=c_char,len=1), intent(out) :: buf(*)
        integer(c_size_t), value :: buf_len
        character(kind=c_char,len=1), value, intent(in) :: empty_char
      end function ioda_vecstring_c_get2_f

      integer(c_size_t) function ioda_vecstring_c_set_f(obj, n, buf, buf_len) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: n
        character(kind=c_char,len=1), intent(in) :: buf(*)
        integer(c_size_t), value :: buf_len
      end function ioda_vecstring_c_set_f

      integer(c_size_t) function ioda_vecstring_c_elementsize_f(obj, n) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: n
      end function ioda_vecstring_c_elementsize_f
  
      integer(c_size_t) function ioda_vecstring_c_size_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end function ioda_vecstring_c_size_f

      subroutine ioda_vecstring_c_resize_f(obj, n) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: n
      end subroutine ioda_vecstring_c_resize_f
  
    end interface
  
    type :: ioda_vecstring
      type(c_ptr) :: data_c = c_null_ptr
    contains
      procedure :: clear => ioda_vecstring_clear
      procedure :: ioda_vecstring_getAsCharArray_4
      procedure :: ioda_vecstring_getAsCharArray_8
      generic :: getAsCharArray => ioda_vecstring_getAsCharArray_4, ioda_vecstring_getAsCharArray_8
      procedure :: ioda_vecstring_setFromCharArray_4
      procedure :: ioda_vecstring_setFromCharArray_8
      generic :: setFromCharArray => ioda_vecstring_setFromCharArray_4, ioda_vecstring_setFromCharArray_8
      procedure :: ioda_vecstring_elementSize_4
      procedure :: ioda_vecstring_elementSize_8
      generic :: elementSize => ioda_vecstring_elementSize_4, ioda_vecstring_elementSize_8
      procedure :: size => ioda_vecstring_size
      procedure :: ioda_vecstring_resize_8
      procedure :: ioda_vecstring_resize_4
      generic :: resize => ioda_vecstring_resize_8, ioda_vecstring_resize_4
      final :: ioda_vecstring_destructor
      procedure, pass(this) :: ioda_vecstring_copy
      generic :: assignment(=) => ioda_vecstring_copy
    end type ioda_vecstring
  
    interface ioda_vecstring
      module procedure ioda_vecstring_constructor
    end interface ioda_vecstring
  
    contains
  
    function ioda_vecstring_constructor(ptr) Result(s)
      type(c_ptr), optional :: ptr
      type(ioda_vecstring) :: s
      type(c_ptr) :: ioda_c
      type(ioda_c_interface_t), pointer :: ioda_c_fp
      type(ioda_vecstring_c), pointer :: vecstringc
      procedure(ioda_vecstring_c_construct_f), pointer :: ctor
  
      if (present(ptr)) then
        s%data_c = ptr
      else
        ioda_c = get_ioda_c_interface()
        call c_f_pointer(ioda_c, ioda_c_fp)
        call c_f_pointer(ioda_c_fp%VecStrings, vecstringc)
        call c_f_procpointer(vecstringc%construct, ctor)
        s%data_c = ctor()
      end if
    end function ioda_vecstring_constructor
  
    subroutine ioda_vecstring_destructor(this)
      implicit none
      type(ioda_vecstring), intent(inout) :: this
      procedure(ioda_vecstring_c_destruct_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
  
      if(c_associated(this%data_c) ) then
        call c_f_pointer(this%data_c, data_f)
        call c_f_procpointer(data_f%destruct, f)
        call f(this%data_c)
        this%data_c = c_null_ptr
      end if
    end subroutine ioda_vecstring_destructor
  
    subroutine ioda_vecstring_clear(this)
      implicit none
      class(ioda_vecstring), intent(inout) :: this
  
      procedure(ioda_vecstring_c_clear_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%clear, f)
      call f(this%data_c)
    end subroutine ioda_vecstring_clear
  
    integer(c_size_t) function ioda_vecstring_getAsCharArray_8(this, n, buf) Result(sz)
      implicit none
      class(ioda_vecstring), intent(in) :: this
      integer(c_size_t) :: n
      character(kind=c_char,len=*), intent(out) :: buf
  
      procedure(ioda_vecstring_c_get2_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%getAsCharArray2, f)
      sz = f(this%data_c, n, buf, len(buf, c_size_t), ' ')
    end function ioda_vecstring_getAsCharArray_8

    integer(c_size_t) function ioda_vecstring_getAsCharArray_4(this, n, buf) Result(sz)
      implicit none
      class(ioda_vecstring), intent(in) :: this
      integer :: n
      character(kind=c_char,len=*), intent(out) :: buf
  
      procedure(ioda_vecstring_c_get2_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%getAsCharArray2, f)
      sz = f(this%data_c, int(n, c_size_t), buf, len(buf, c_size_t), ' ')
    end function ioda_vecstring_getAsCharArray_4
  
    integer(c_size_t) function ioda_vecstring_setFromCharArray_8(this, n, buf) Result(sz)
      implicit none
      class(ioda_vecstring), intent(inout) :: this
      integer(c_size_t) :: n
      character(kind=c_char,len=*), intent(in) :: buf
  
      procedure(ioda_vecstring_c_set_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%setFromCharArray, f)
      sz = f(this%data_c, n, buf, len(buf, c_size_t))
    end function ioda_vecstring_setFromCharArray_8

    integer(c_size_t) function ioda_vecstring_setFromCharArray_4(this, n, buf) Result(sz)
      implicit none
      class(ioda_vecstring), intent(inout) :: this
      integer :: n
      character(kind=c_char,len=*), intent(in) :: buf
  
      procedure(ioda_vecstring_c_set_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%setFromCharArray, f)
      sz = f(this%data_c, int(n, c_size_t), buf, len(buf, c_size_t))
    end function ioda_vecstring_setFromCharArray_4
  
    integer(c_size_t) function ioda_vecstring_elementsize_8(this, n) Result(sz)
      implicit none
      class(ioda_vecstring), intent(in) :: this
      integer(c_size_t) :: n
  
      procedure(ioda_vecstring_c_elementsize_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%elementsize, f)
      sz = f(this%data_c, n)
    end function ioda_vecstring_elementsize_8

    integer(c_size_t) function ioda_vecstring_elementsize_4(this, n) Result(sz)
      implicit none
      class(ioda_vecstring), intent(in) :: this
      integer :: n
  
      procedure(ioda_vecstring_c_elementsize_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%elementsize, f)
      sz = f(this%data_c, int(n, c_size_t))
    end function ioda_vecstring_elementsize_4
    
    integer(c_size_t) function ioda_vecstring_size(this) Result(sz)
      implicit none
      class(ioda_vecstring), intent(in) :: this
  
      procedure(ioda_vecstring_c_size_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%size, f)
      sz = f(this%data_c)
    end function ioda_vecstring_size

    subroutine ioda_vecstring_resize_8(this, n)
      implicit none
      class(ioda_vecstring), intent(in) :: this
      integer(c_size_t) :: n
  
      procedure(ioda_vecstring_c_resize_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%resize, f)
      call f(this%data_c, n)
    end subroutine ioda_vecstring_resize_8

    subroutine ioda_vecstring_resize_4(this, n)
      implicit none
      class(ioda_vecstring), intent(in) :: this
      integer :: n
    
      procedure(ioda_vecstring_c_resize_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%resize, f)
      call f(this%data_c, int(n, c_size_t))
    end subroutine ioda_vecstring_resize_4
  
    subroutine ioda_vecstring_copy(this, from)
      class(ioda_vecstring), intent(inout) :: this
      class(ioda_vecstring), intent(in) :: from
  
      type(c_ptr) :: ioda_c
      type(ioda_c_interface_t), pointer :: ioda_c_fp
      type(ioda_vecstring_c), pointer :: vecstringc
      procedure(ioda_vecstring_c_construct_f), pointer :: ctor
  
      procedure(ioda_vecstring_c_copy_f), pointer :: f
      type(ioda_vecstring_c), pointer :: data_f => NULL()
  
      ! Slightly awkwardly constructing a string to get pointers
      ! to later remove the string. Limitation of the C interface.
      if (.not. c_associated(this%data_c)) then
        ioda_c = get_ioda_c_interface()
        call c_f_pointer(ioda_c, ioda_c_fp)
        call c_f_pointer(ioda_c_fp%VecStrings, vecstringc)
        call c_f_procpointer(vecstringc%construct, ctor)
        this%data_c = ctor()
      end if
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%copy, f)
  
      call ioda_vecstring_destructor(this)
      if (c_associated(from%data_c)) then
        this%data_c = f(from%data_c)
      end if
    end subroutine ioda_vecstring_copy
  
  
  end module ioda_vecstring_f
