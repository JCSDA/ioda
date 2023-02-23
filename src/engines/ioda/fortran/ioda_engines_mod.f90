module ioda_engines_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env
   use :: ioda_group_mod
   use :: ioda_vecstring_mod
   use :: ioda_f_c_string_mod
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

      function ioda_engines_c_init(fckit,tb,te) result(ptr) bind(C,name="ioda_engines_c_init") 
      	 import c_ptr
      	 type(c_ptr) :: ptr
      end function
      
      subroutine ioda_engines_c_dealloc(p) bind(C,name="ioda_engines_c_dealloc")
      	 import c_ptr
      	 type(c_ptr) :: p
      end subroutine

      function ioda_engines_c_create_root_group() result(grp) bind(C,name="ioda_engines_c_create_root_group")
         import c_ptr
         type(c_ptr) :: grp 
      end function 
             
   end interface
contains
   subroutine ioda_engines_hh_create_memory_file(new_grp, fname, increment_len)
      implicit none
      class(ioda_group), intent(out) :: new_grp
      character(len=*), intent(in) :: fname
      integer(int64), intent(in) :: increment_len
      type(C_ptr) :: fname_p

      if (c_associated(new_grp%data_ptr)) then
         call ioda_group_dtor(new_grp)
      end if
      fname_p = ioda_f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_create_memory_file(fname_p, increment_len)
      call ioda_c_free(fname_p)
   end subroutine

   subroutine ioda_engines_hh_open_memory_file(new_grp, fname, increment_len)
      implicit none
      class(ioda_group), intent(out) :: new_grp
      character(len=*), intent(in) :: fname
      integer(int64), intent(in) :: increment_len
      type(C_ptr) :: fname_p

      if (c_associated(new_grp%data_ptr)) then
         call ioda_group_dtor(new_grp)
      end if
      fname_p = ioda_f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_open_memory_file(fname_p, increment_len)
      call ioda_c_free(fname_p)
   end subroutine

   subroutine ioda_engines_hh_open_file(new_grp, fname, open_mode)
      implicit none
      class(ioda_group) :: new_grp
      integer(int64) :: sz_fname
      character(len=*), intent(in) :: fname
      integer(int32) :: open_mode
      type(C_ptr) :: fname_p

      if (c_associated(new_grp%data_ptr)) then
         call ioda_group_dtor(new_grp)
      end if
      if (open_mode > 3) open_mode = 0
      if (open_mode < 0) open_mode = 0
      fname_p = ioda_f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_open_file(fname_p, open_mode)
      call ioda_c_free(fname_p)
   end subroutine

   subroutine ioda_engines_hh_create_file(new_grp, fname, create_mode)
      implicit none
      class(ioda_group) :: new_grp
      integer(int64) :: sz_fname
      character(len=*), intent(in) :: fname
      integer(int32) :: create_mode
      type(C_ptr) :: fname_p

      if (c_associated(new_grp%data_ptr)) then
         call ioda_group_dtor(new_grp)
      end if
      if (create_mode > 3) create_mode = 0
      if (create_mode < 0) create_mode = 0
      call ioda_group_init(new_grp)
      fname_p = ioda_f_string_to_c_dup(fname)
      new_grp%data_ptr = ioda_engines_c_hh_create_file(fname_p, create_mode)
      call ioda_c_free(fname_p)
   end subroutine

   subroutine ioda_engines_construct_from_command_line(new_grp, default_filename)
      implicit none
      class(ioda_group) :: new_grp
      character(len=*), intent(in) :: default_filename
      type(ioda_vecstring) :: vstr
      type(c_ptr) :: default_filename_p

      if (c_associated(new_grp%data_ptr)) then
         call ioda_group_dtor(new_grp)
      end if
      call ioda_vecstring_init(vstr)
      call ioda_command_line_to_vecstring(vstr)
      write (error_unit, *) 'ctor cmd line'
      write (error_unit, *) default_filename
      write (error_unit, *) len_trim(default_filename)
      write (error_unit, *) len(default_filename)
      write (error_unit, *) 'done'
      default_filename_p = ioda_f_string_to_c_dup(default_filename)
      new_grp%data_ptr = ioda_engines_c_construct_from_command_line(vstr%data_ptr, default_filename_p)
      call ioda_c_free(default_filename_p)
   end subroutine

!   function ioda_engines_create_root_group() result(grp)
!      implicit none
!      class(ioda_group) :: grp
!      grp%data_ptr  = ioda_engines_c_create_root_group()
!   end function

!!
!!   Create a ioda_group from the group pointer returned from obsspace construction
!!
   subroutine ioda_engines_init_from_obstore_ptr(this,obs_group_ptr)
   	implicit none
   	class(ioda_group) :: this
   	type(c_ptr) :: obs_group_ptr
   	this%data_ptr = obs_group_ptr
   end subroutine  
end module
