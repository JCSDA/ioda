!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  @file ioda_string.f90
!>  @brief Fortran interface to ioda strings. Used to encapsulate C++ strings.

!>  @brief Fortran interface to ioda strings. Used to encapsulate C++ strings.
module ioda_string_f
    use, intrinsic :: iso_c_binding
    use :: ioda_f
    implicit none
  
    public
  
    type, bind(C) :: ioda_string_c
      type(c_funptr) :: construct
      type(c_funptr) :: constructFromCstr
      type(c_funptr) :: destruct
      type(c_funptr) :: clear
      type(c_funptr) :: get
      type(c_funptr) :: length
      type(c_funptr) :: set
      type(c_funptr) :: size
      type(c_funptr) :: copy
      type(c_ptr)    :: data_
    end type ioda_string_c
  
    abstract interface
      type(c_ptr) function ioda_string_c_construct_f() bind(C)
        use, intrinsic :: iso_c_binding
      end function ioda_string_c_construct_f
  
      type(c_ptr) function ioda_string_c_construct_from_c_str_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end function ioda_string_c_construct_from_c_str_f
  
      type(c_ptr) function ioda_string_c_copy_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end function ioda_string_c_copy_f
  
      subroutine ioda_string_c_destruct_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end subroutine ioda_string_c_destruct_f
  
      subroutine ioda_string_c_clear_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end subroutine ioda_string_c_clear_f
  
      integer(c_size_t) function ioda_string_c_get_f(obj, buf, buf_len) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        character(kind=c_char,len=1), intent(out) :: buf(*)
        integer(c_size_t), value :: buf_len
      end function ioda_string_c_get_f
  
      integer(c_size_t) function ioda_string_c_size_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end function ioda_string_c_size_f
  
      integer(c_size_t) function ioda_string_c_set_f(obj, buf, buf_len) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        character(kind=c_char,len=1), intent(in) :: buf(*)
        integer(c_size_t), value :: buf_len
      end function ioda_string_c_set_f
  
    end interface
  
    type :: ioda_string
      type(c_ptr) :: data_c = c_null_ptr
    contains
      procedure :: clear => ioda_string_clear
      procedure :: get => ioda_string_get
      procedure :: set => ioda_string_set
      procedure :: size => ioda_string_size
      final :: ioda_string_destructor
      procedure, pass(this) :: ioda_string_copy
      generic :: assignment(=) => ioda_string_copy
    end type ioda_string
  
    interface ioda_string
      module procedure ioda_string_constructor
    end interface ioda_string
  
    contains
  
    !> \param ptr optionally refers to an existing ioda_string* that
    !>    needs encapsulization. If passed, then the Fortran ioda_string
    !>    object takes ownership of ptr.
    function ioda_string_constructor(ptr) Result(s)
      type(c_ptr), optional :: ptr
  
      type(ioda_string) :: s
      type(c_ptr) :: ioda_c                                 =  c_null_ptr
      type(ioda_c_interface_t), pointer :: ioda_c_fp        => null()
      type(ioda_string_c), pointer :: stringc               => null()
      procedure(ioda_string_c_construct_f), pointer :: ctor => null()
  
      if (present(ptr)) then
        s%data_c = ptr
      else
        ioda_c = get_ioda_c_interface()
        call c_f_pointer(ioda_c, ioda_c_fp)
        call c_f_pointer(ioda_c_fp%strings, stringc)
        call c_f_procpointer(stringc%construct, ctor)
        s%data_c = ctor()
      end if
    end function ioda_string_constructor
  
    function ioda_string_constructor_cstr(ptr) Result(s)
      type(c_ptr) :: ptr
      type(ioda_string) :: s
  
      type(c_ptr) :: ioda_c
      type(ioda_c_interface_t), pointer :: ioda_c_fp
      type(ioda_string_c), pointer :: stringc
      procedure(ioda_string_c_construct_from_c_str_f), pointer :: ctor
  
      ioda_c = get_ioda_c_interface()
      call c_f_pointer(ioda_c, ioda_c_fp)
      call c_f_pointer(ioda_c_fp%Strings, stringc)
      call c_f_procpointer(stringc%constructFromCstr, ctor)
      s%data_c = ctor(ptr)
    end function ioda_string_constructor_cstr
  
    subroutine ioda_string_destructor(this)
      implicit none
      type(ioda_string), intent(inout) :: this
      procedure(ioda_string_c_destruct_f), pointer :: f
      type(ioda_string_c), pointer :: data_f => NULL()
  
      if(c_associated(this%data_c) ) then
        call c_f_pointer(this%data_c, data_f)
        call c_f_procpointer(data_f%destruct, f)
        call f(this%data_c)
        this%data_c = c_null_ptr
      end if
    end subroutine ioda_string_destructor
  
    subroutine ioda_string_clear(this)
      implicit none
      class(ioda_string), intent(inout) :: this
  
      procedure(ioda_string_c_clear_f), pointer :: f
      type(ioda_string_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%clear, f)
      call f(this%data_c)
    end subroutine ioda_string_clear
  
    integer(c_size_t) function ioda_string_get(this, buf) Result(sz)
      implicit none
      class(ioda_string), intent(in) :: this
      character(kind=c_char,len=*), intent(out) :: buf
  
      procedure(ioda_string_c_get_f), pointer :: f
      type(ioda_string_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%get, f)
      sz = f(this%data_c, buf, len(buf, c_size_t))
    end function ioda_string_get
  
    subroutine ioda_string_set(this, buf)
      implicit none
      class(ioda_string), intent(inout) :: this
      character(kind=c_char,len=*), intent(in) :: buf
  
      integer(c_size_t) :: sz
      procedure(ioda_string_c_set_f), pointer :: f
      type(ioda_string_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%set, f)
      sz = f(this%data_c, buf, len(buf, c_size_t))
    end subroutine ioda_string_set
  
    integer(c_size_t) function ioda_string_size(this) Result(sz)
      implicit none
      class(ioda_string), intent(in) :: this
  
      procedure(ioda_string_c_size_f), pointer :: f
      type(ioda_string_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%size, f)
      sz = f(this%data_c)
    end function ioda_string_size
  
    subroutine ioda_string_copy(this, from)
      class(ioda_string), intent(inout) :: this
      class(ioda_string), intent(in) :: from
  
      type(c_ptr) :: ioda_c = c_null_ptr
      type(ioda_c_interface_t), pointer :: ioda_c_fp => null()
      type(ioda_string_c), pointer :: stringc => null()
      procedure(ioda_string_c_construct_f), pointer :: ctor => null()
  
      procedure(ioda_string_c_copy_f), pointer :: f => null()
      type(ioda_string_c), pointer :: data_f => null()
  
      ! Slightly awkwardly constructing a string to get pointers
      ! to later remove the string. Limitation of the C interface.
      if (.not. c_associated(this%data_c)) then
        ioda_c = get_ioda_c_interface()
        call c_f_pointer(ioda_c, ioda_c_fp)
        call c_f_pointer(ioda_c_fp%Strings, stringc)
        call c_f_procpointer(stringc%construct, ctor)
        this%data_c = ctor()
      end if
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%copy, f)
  
      call ioda_string_destructor(this)
      if (c_associated(from%data_c)) then
        this%data_c = f(from%data_c)
      end if
    end subroutine ioda_string_copy
  
  
  end module ioda_string_f
