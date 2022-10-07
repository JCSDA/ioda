!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  @file ioda_group.f90
!>  @brief Fortran interface to ioda Group.

!>  @brief Fortran interface to ioda Group.
module ioda_group_f
    use, intrinsic :: iso_c_binding
    use :: ioda_f
    use :: ioda_vecstring_f
    implicit none
  
    public
  
    type, bind(C) :: ioda_group_c
      type(c_funptr) :: destruct
      type(c_funptr) :: list
      type(c_funptr) :: exists
      type(c_funptr) :: create
      type(c_funptr) :: open
      type(c_funptr) :: clone
      type(c_ptr)    :: atts
      type(c_ptr)    :: vars
      type(c_ptr)    :: grp
    end type ioda_group_c
  
    abstract interface
      subroutine ioda_group_c_destruct_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end subroutine ioda_group_c_destruct_f

      type(c_ptr) function ioda_group_c_list_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end function ioda_group_c_list_f
  
      logical(c_bool) function ioda_group_c_exists_f(obj, child_sz, child_name) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: child_sz
        character(kind=c_char,len=1), intent(in) :: child_name(*)
      end function ioda_group_c_exists_f

      type(c_ptr) function ioda_group_c_create_f(obj, child_sz, child_name) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: child_sz
        character(kind=c_char,len=1), intent(in) :: child_name(*)
      end function ioda_group_c_create_f

      type(c_ptr) function ioda_group_c_open_f(obj, child_sz, child_name) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
        integer(c_size_t), value :: child_sz
        character(kind=c_char,len=1), intent(in) :: child_name(*)
      end function ioda_group_c_open_f

      type(c_ptr) function ioda_group_c_clone_f(obj) bind(C)
        use, intrinsic :: iso_c_binding
        type(c_ptr), value :: obj
      end function ioda_group_c_clone_f
  
    end interface
  
    type :: ioda_group
      type(c_ptr) :: data_c = c_null_ptr
    contains
      procedure :: list => ioda_group_list
      procedure :: exists => ioda_group_exists
      procedure :: create => ioda_group_create
      procedure :: open => ioda_group_open
      final :: ioda_group_destructor
      procedure, pass(this) :: ioda_group_copy
      generic :: assignment(=) => ioda_group_copy
    end type ioda_group
  
    interface ioda_group
      module procedure ioda_group_constructor
    end interface ioda_group
  
    contains
  
    function ioda_group_constructor(ptr) Result(s)
      type(ioda_group) :: s
      type(c_ptr) :: ptr
  
      s%data_c = ptr
    end function ioda_group_constructor
  
    subroutine ioda_group_destructor(this)
      type(ioda_group), intent(inout) :: this
      procedure(ioda_group_c_destruct_f), pointer :: f
      type(ioda_group_c), pointer :: data_f => NULL()
  
      if(c_associated(this%data_c) ) then
        call c_f_pointer(this%data_c, data_f)
        call c_f_procpointer(data_f%destruct, f)
        call f(this%data_c)
        this%data_c = c_null_ptr
      end if
    end subroutine ioda_group_destructor

    type(ioda_vecstring) function ioda_group_list(this) Result(list)
      class(ioda_group), intent(in) :: this
      procedure(ioda_group_c_list_f), pointer :: f

      type(ioda_group_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%list, f)
      list = ioda_vecstring(f(this%data_c))
    end function ioda_group_list

    logical(c_bool) function ioda_group_exists(this, name) Result(res)
      class(ioda_group), intent(in) :: this
      character(kind=c_char,len=*), intent(in) :: name
      procedure(ioda_group_c_exists_f), pointer :: f

      type(ioda_group_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%exists, f)
      res = f(this%data_c, len(name, c_size_t), name)
    end function ioda_group_exists

    type(ioda_group) function ioda_group_create(this, name) Result(res)
      class(ioda_group), intent(in) :: this
      character(kind=c_char,len=*), intent(in) :: name
      procedure(ioda_group_c_create_f), pointer :: f

      type(ioda_group_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%create, f)
      res = ioda_group(f(this%data_c, len(name, c_size_t), name))
    end function ioda_group_create

    type(ioda_group) function ioda_group_open(this, name) Result(res)
      class(ioda_group), intent(in) :: this
      character(kind=c_char,len=*), intent(in) :: name
      procedure(ioda_group_c_open_f), pointer :: f

      type(ioda_group_c), pointer :: data_f => NULL()
      call c_f_pointer(this%data_c, data_f)
      call c_f_procpointer(data_f%open, f)
      res = ioda_group(f(this%data_c, len(name, c_size_t), name))
    end function ioda_group_open
  
    subroutine ioda_group_copy(this, from)
      class(ioda_group), intent(inout) :: this
      class(ioda_group), intent(in) :: from
  
      !type(c_ptr) :: ioda_c
      !type(ioda_c_interface_t), pointer :: ioda_c_fp
      !type(ioda_group_c), pointer :: groupc
      !procedure(ioda_group_c_construct_f), pointer :: ctor
  
      procedure(ioda_group_c_clone_f), pointer :: f
      type(ioda_group_c), pointer :: data_f => NULL()
  
      ! Slightly awkwardly constructing a string to get pointers
      ! to later remove the string. Limitation of the C interface.
      if (c_associated(this%data_c)) then
        call ioda_group_destructor(this)
      end if
      !ioda_c = get_ioda_c_interface()
      !call c_f_pointer(ioda_c, ioda_c_fp)
      !call c_f_pointer(ioda_c_fp%VecStrings, vecstringc)
      !call c_f_procpointer(vecstringc%construct, ctor)

      if (c_associated(from%data_c)) then
        call c_f_pointer(from%data_c, data_f)
        call c_f_procpointer(data_f%clone, f)
        this%data_c = f(from%data_c)
      end if
    end subroutine ioda_group_copy
  
end module ioda_group_f
