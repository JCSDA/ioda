!
! (C) Copyright 2023 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module ioda_engines_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env
   use :: ioda_group_mod
   use :: cxx_vector_string_mod
   use :: f_c_string_mod
   interface
      function ioda_engines_c_obstore_create_root_group() result(new_grp) &
      & bind(C, name="ioda_engines_c_obstore_create_root_group")
         import c_ptr
         type(c_ptr) :: new_grp
      end function

      function ioda_engines_c_hh_create_memory_file(name, increment_len) result(new_grp) &
      & bind(C, name="ioda_engines_c_hh_create_memory_file")
         import c_ptr, c_int64_t
         type(c_ptr) :: new_grp
         type(c_ptr), value :: name
         integer(c_int64_t), value :: increment_len
      end function

      function ioda_engines_c_hh_open_memory_file(name, increment_len) result(new_grp) &
      & bind(C, name="ioda_engines_c_hh_open_memory_file")
         import c_ptr, c_int64_t
         type(c_ptr) :: new_grp
         type(c_ptr), value :: name
         integer(c_int64_t), value :: increment_len
      end function

      function ioda_engines_c_hh_open_file(name, backend_mode) result(new_grp) &
      & bind(C, name="ioda_engines_c_hh_open_file")
         import c_ptr, c_int32_t
         type(c_ptr) :: new_grp
         type(c_ptr), value :: name
         integer(c_int32_t), value :: backend_mode
      end function

      function ioda_engines_c_hh_create_file(name, backend_mode) result(new_grp) &
      & bind(C, name="ioda_engines_c_hh_create_file")
         import c_ptr, c_int32_t
         type(c_ptr) :: new_grp
         type(c_ptr), value :: name
         integer(c_int32_t), value :: backend_mode
      end function

      function ioda_engines_c_construct_from_command_line(vstrs, default_filename) result(new_grp) &
      & bind(C, name="ioda_engines_c_construct_from_command_line")
         import c_ptr
         type(c_ptr) :: new_grp
         type(c_ptr), value :: default_filename
         type(c_ptr), value :: vstrs
      end function
   end interface
contains
   function ioda_engines_obstore_create_root_group() result(new_grp)
      implicit none
      type(ioda_group) :: new_grp
      new_grp%data_ptr = ioda_engines_c_obstore_create_root_group()
   end function

   function ioda_engines_hh_create_memory_file(fname, increment_len) result(new_grp)
      implicit none
      type(ioda_group) :: new_grp
      character(len=*), intent(in) :: fname
      integer(int64), intent(in) :: increment_len
      type(C_ptr) :: fname_p

      fname_p = f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_create_memory_file(fname_p, increment_len)
      call c_free(fname_p)
   end function

   function ioda_engines_hh_open_memory_file(fname, increment_len) result(new_grp)
      implicit none
      type(ioda_group) :: new_grp
      character(len=*), intent(in) :: fname
      integer(int64), intent(in) :: increment_len
      type(C_ptr) :: fname_p

      fname_p = f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_open_memory_file(fname_p, increment_len)
      call c_free(fname_p)
   end function

   function ioda_engines_hh_open_file(fname, open_mode) result(new_grp)
      implicit none
      type(ioda_group) :: new_grp
      integer(int64) :: sz_fname
      character(len=*), intent(in) :: fname
      integer(int32) :: open_mode
      type(C_ptr) :: fname_p

      if (open_mode > 3) open_mode = 0
      if (open_mode < 0) open_mode = 0
      fname_p = f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_open_file(fname_p, open_mode)
      call c_free(fname_p)
   end function

   function ioda_engines_hh_create_file(fname, create_mode) result(new_grp)
      implicit none
      type(ioda_group) :: new_grp
      integer(int64) :: sz_fname
      character(len=*), intent(in) :: fname
      integer(int32) :: create_mode
      type(C_ptr) :: fname_p

      if (create_mode > 3) create_mode = 0
      if (create_mode < 0) create_mode = 0
      fname_p = f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_create_file(fname_p, create_mode)
      call c_free(fname_p)
   end function

   subroutine ioda_command_line_to_vecstring(vstring)
      implicit none
      class(cxx_vector_string), intent(inout) :: vstring
      integer :: argc
      character(len=512) :: argv
      integer :: i

      if (.not. c_associated(vstring%data_ptr)) then
      	vstring%data_ptr = cxx_vector_string_c_alloc()
      else
      	call vstring%clear()
      end if
      argc = command_argument_count()
      do i = 1, argc
         call get_command_argument(i, argv)
         call vstring%push_back(argv)
      end do
   end subroutine

   function ioda_engines_construct_from_command_line(default_filename) result(new_grp)
      implicit none
      type(ioda_group) :: new_grp
      character(len=*), intent(in) :: default_filename
      type(cxx_vector_string) :: vstr
      type(c_ptr) :: default_filename_p
      
      call cxx_vector_string_init(vstr)
      call ioda_command_line_to_vecstring(vstr)
      default_filename_p = f_string_to_c_dup(default_filename)
      new_grp%data_ptr = ioda_engines_c_construct_from_command_line(vstr%data_ptr, default_filename_p)
      call c_free(default_filename_p)
   end function
end module
