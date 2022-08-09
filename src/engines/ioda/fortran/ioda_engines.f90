!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  @file ioda_engines.f90
!>  @brief Fortran interface to ioda engines.

!>  @brief Fortran interface to ioda engines.
module ioda_engines_f
    use, intrinsic :: iso_c_binding
    use :: ioda_f
    !use :: ioda_vecstring_f
    use :: ioda_group_f
    implicit none
  
    public

    ! Fortran lacks named enumerators
    enum, bind(c)
      enumerator ioda_Engines_BackendOpenModes_Read_Only
      enumerator ioda_Engines_BackendOpenModes_Read_Write
    end enum

    enum, bind(c)
      enumerator ioda_Engines_BackendCreateModes_Truncate_If_Exists
      enumerator ioda_Engines_BackendCreateModes_Fail_If_Exists
    end enum
  
    type, bind(C) :: ioda_engines_obsstore_c
      type(c_funptr) :: createRootGroup
    end type ioda_engines_obsstore_c

    type, bind(C) :: ioda_engines_hh_c
      type(c_funptr) :: createMemoryFile
      type(c_funptr) :: openFile
      type(c_funptr) :: createFile
    end type ioda_engines_hh_c

    type, bind(C) :: ioda_engines_c
      type(c_funptr) :: constructFromCmdLine
      type(c_ptr)    :: hh
      type(c_ptr)    :: obsstore
    end type ioda_engines_c
  
    abstract interface
      type(c_ptr) function ioda_engines_obsstore_c_createrootgroup_f() bind(C)
        use, intrinsic :: iso_c_binding
      end function ioda_engines_obsstore_c_createrootgroup_f

      type(c_ptr) function ioda_engines_hh_c_creatememoryfile_f(name_sz, name, flush, inc) bind(C)
        use, intrinsic :: iso_c_binding
        integer(c_size_t), value :: name_sz
        character(kind=c_char,len=1), intent(in) :: name(*)
        logical(c_bool), value :: flush
        integer(c_long), value :: inc
      end function ioda_engines_hh_c_creatememoryfile_f

      type(c_ptr) function ioda_engines_hh_c_openfile_f(name_sz, name, openmode) bind(C)
        use, intrinsic :: iso_c_binding
        integer(c_size_t), value :: name_sz
        character(kind=c_char,len=1), intent(in) :: name(*)
        integer(c_int), value :: openmode
      end function ioda_engines_hh_c_openfile_f
      
      type(c_ptr) function ioda_engines_hh_c_createfile_f(name_sz, name, createmode) bind(C)
        use, intrinsic :: iso_c_binding
        integer(c_size_t), value :: name_sz
        character(kind=c_char,len=1), intent(in) :: name(*)
        integer(c_int), value :: createmode
      end function ioda_engines_hh_c_createfile_f
  
    end interface
  
    type :: ioda_engines_obsstore
    contains
      procedure, nopass :: createRootGroup => ioda_engines_obsstore_createRootGroup
    end type ioda_engines_obsstore

    type :: ioda_engines_hh
    contains
      procedure, nopass :: creatememoryfile => ioda_engines_hh_creatememoryfile
      procedure, nopass :: openfile => ioda_engines_hh_openfile
      procedure, nopass :: createfile => ioda_engines_hh_createfile
    end type ioda_engines_hh
    
    type :: ioda_engines
      type(ioda_engines_obsstore) :: obsstore
      type(ioda_engines_hh) :: hh
    end type ioda_engines
  
    !interface ioda_engines
    !  module procedure ioda_engines_constructor
    !end interface ioda_engines
  
    contains
  
    !function ioda_engines_constructor() Result(s)
    !  type(ioda_engines) :: s
    !end function ioda_engines_constructor

    function ioda_engines_obsstore_createRootGroup() Result(res)
      type(ioda_group) :: res
      type(c_ptr) :: ioda_c
      type(ioda_c_interface_t), pointer :: ioda_c_fp
      type(ioda_engines_c), pointer :: engines_c
      type(ioda_engines_obsstore_c), pointer :: obsstore_c
      procedure(ioda_engines_obsstore_c_createrootgroup_f), pointer :: f

      ioda_c = get_ioda_c_interface()
      call c_f_pointer(ioda_c, ioda_c_fp)
      call c_f_pointer(ioda_c_fp%Engines, engines_c)
      call c_f_pointer(engines_c%obsstore, obsstore_c)
      call c_f_procpointer(obsstore_c%createRootGroup, f)

      res = ioda_group(f())
    end function ioda_engines_obsstore_createRootGroup

    function ioda_engines_hh_creatememoryfile(name, flush, inc) Result(res)
        character(kind=c_char,len=*), intent(in) :: name
        logical(c_bool), value :: flush
        integer(c_long), value :: inc
        type(ioda_group) :: res
        type(c_ptr) :: ioda_c
        type(ioda_c_interface_t), pointer :: ioda_c_fp
        type(ioda_engines_c), pointer :: engines_c
        type(ioda_engines_hh_c), pointer :: hh_c
        procedure(ioda_engines_hh_c_creatememoryfile_f), pointer :: f

        ioda_c = get_ioda_c_interface()
        call c_f_pointer(ioda_c, ioda_c_fp)
        call c_f_pointer(ioda_c_fp%Engines, engines_c)
        call c_f_pointer(engines_c%hh, hh_c)
        call c_f_procpointer(hh_c%createMemoryFile, f)
  
        res = ioda_group(f(len(name, c_size_t), name, flush, inc))
      end function ioda_engines_hh_creatememoryfile

      function ioda_engines_hh_openfile(name, mode) Result(res)
        character(kind=c_char,len=*), intent(in) :: name
        integer(c_int), value :: mode
        type(ioda_group) :: res
        type(c_ptr) :: ioda_c
        type(ioda_c_interface_t), pointer :: ioda_c_fp
        type(ioda_engines_c), pointer :: engines_c
        type(ioda_engines_hh_c), pointer :: hh_c
        procedure(ioda_engines_hh_c_openfile_f), pointer :: f

        ioda_c = get_ioda_c_interface()
        call c_f_pointer(ioda_c, ioda_c_fp)
        call c_f_pointer(ioda_c_fp%Engines, engines_c)
        call c_f_pointer(engines_c%hh, hh_c)
        call c_f_procpointer(hh_c%openFile, f)
  
        res = ioda_group(f(len(name, c_size_t), name, mode))
      end function ioda_engines_hh_openfile

      function ioda_engines_hh_createfile(name, mode) Result(res)
        character(kind=c_char,len=*), intent(in) :: name
        integer(c_int), value :: mode
        type(ioda_group) :: res
        type(c_ptr) :: ioda_c
        type(ioda_c_interface_t), pointer :: ioda_c_fp
        type(ioda_engines_c), pointer :: engines_c
        type(ioda_engines_hh_c), pointer :: hh_c
        procedure(ioda_engines_hh_c_createfile_f), pointer :: f

        ioda_c = get_ioda_c_interface()
        call c_f_pointer(ioda_c, ioda_c_fp)
        call c_f_pointer(ioda_c_fp%Engines, engines_c)
        call c_f_pointer(engines_c%hh, hh_c)
        call c_f_procpointer(hh_c%createFile, f)
  
        res = ioda_group(f(len(name, c_size_t), name, mode))
      end function ioda_engines_hh_createfile
  
end module ioda_engines_f
