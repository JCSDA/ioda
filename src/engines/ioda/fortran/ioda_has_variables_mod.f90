module ioda_has_variables_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env
   use :: ioda_variable_creation_parameters_mod
   use :: ioda_variable_mod
   use :: ioda_vecstring_mod
   use :: ioda_f_c_string_mod

   type :: ioda_has_variables
      type(c_ptr) :: data_ptr
   contains
      final ioda_has_variables_dtor

      procedure, private, pass(this) ::ioda_has_variables_copy
      generic, public :: assignment(=) => ioda_has_variables_copy

      procedure :: open => ioda_has_variables_open
      procedure :: exists => ioda_has_variables_exists
      procedure :: remove => ioda_has_variables_remove
      procedure :: list => ioda_has_variables_list
      procedure :: create_float => ioda_has_variables_create_float
      procedure :: create_double => ioda_has_variables_create_double
      procedure :: create_char => ioda_has_variables_create_char
      procedure :: create_int16 => ioda_has_variables_create_int16
      procedure :: create_int32 => ioda_has_variables_create_int32
      procedure :: create_int64 => ioda_has_variables_create_int64
      procedure :: create_str => ioda_has_variables_create_str
   end type

   interface
      function ioda_has_variables_c_alloc() result(p) bind(C, name="ioda_has_variables_c_alloc")
         import c_ptr
         type(c_ptr) :: p
      end function

      subroutine ioda_has_variables_c_dtor(p) bind(C, name="ioda_has_variables_c_dtor")
         import c_ptr
         type(c_ptr), value :: p
      end subroutine

      subroutine ioda_has_variables_c_clone(this, rhs) bind(C, name="ioda_has_variables_c_clone")
         import c_ptr
         type(c_ptr), value :: rhs
         type(c_ptr) :: this
      end subroutine

      function ioda_has_variables_c_list(p) result(vstr) bind(C, name="ioda_has_variables_c_list")
         import c_ptr
         type(c_ptr), value :: p
         type(c_ptr) :: vstr
      end function

      function ioda_has_variables_c_remove(p, n, name_p) result(r) bind(C, name="ioda_has_variables_c_remove")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p, name_p
         integer(c_int64_t) :: n
         logical(c_bool) :: r
      end function

      function ioda_has_variables_c_exists(p, n, name_p) result(r) bind(C, name="ioda_has_variables_c_exists")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p, name_p
         integer(c_int64_t) :: n
         logical(c_bool) :: r
      end function

      function ioda_has_variables_c_open(p, n, name_p) result(var_p) bind(C, name="ioda_has_variables_c_open")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p, name_p
         integer(c_int64_t) :: n
         type(c_ptr) :: var_p
         logical(c_bool) :: r
      end function

      function ioda_has_variables_c_create2_float(p, sz_name, name, n_dims, dims, max_dims, c_param) &
      & result(var) bind(C, name="ioda_has_variables_c_create2_float")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr), value :: c_param
         type(c_ptr) :: var
         integer(c_int64_t) :: sz_name
         integer(c_int64_t) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) :: dims
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
      end function

      function ioda_has_variables_c_create_float(p, sz_name, name, n_dims, dims) result(var) &
      & bind(C, name="ioda_has_variables_c_create_float")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz_name
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) ::dims
         type(c_ptr) :: var
      end function

      function ioda_has_variables_c_create2_double(p, sz_name, name, n_dims, dims, max_dims, c_param) &
      & result(var) bind(C, name="ioda_has_variables_c_create2_double")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr), value :: c_param
         type(c_ptr) :: var
         integer(c_int64_t) :: sz_name
         integer(c_int64_t) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) :: dims
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
      end function

      function ioda_has_variables_c_create_double(p, sz_name, name, n_dims, dims) result(var) &
      & bind(C, name="ioda_has_variables_c_create_double")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz_name
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) ::dims
         type(c_ptr) :: var
      end function

      function ioda_has_variables_c_create2_char(p, sz_name, name, n_dims, dims, max_dims, c_param) &
      & result(var) bind(C, name="ioda_has_variables_c_create2_char")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr), value :: c_param
         type(c_ptr) :: var
         integer(c_int64_t) :: sz_name
         integer(c_int64_t) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) :: dims
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
      end function

      function ioda_has_variables_c_create_char(p, sz_name, name, n_dims, dims) result(var) &
      & bind(C, name="ioda_has_variables_c_create_char")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz_name
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) ::dims
         type(c_ptr) :: var
      end function

      function ioda_has_variables_c_create2_int16(p, sz_name, name, n_dims, dims, max_dims, c_param) &
      & result(var) bind(C, name="ioda_has_variables_c_create2_int16")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr), value :: c_param
         type(c_ptr) :: var
         integer(c_int64_t) :: sz_name
         integer(c_int64_t) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) :: dims
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
      end function

      function ioda_has_variables_c_create_int16(p, sz_name, name, n_dims, dims) result(var) &
      & bind(C, name="ioda_has_variables_c_create_int16")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz_name
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) ::dims
         type(c_ptr) :: var
      end function

      function ioda_has_variables_c_create2_int32(p, sz_name, name, n_dims, dims, max_dims, c_param) &
      & result(var) bind(C, name="ioda_has_variables_c_create2_int32")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr), value :: c_param
         type(c_ptr) :: var
         integer(c_int64_t) :: sz_name
         integer(c_int64_t) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) :: dims
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
      end function

      function ioda_has_variables_c_create_int32(p, sz_name, name, n_dims, dims) result(var) &
      & bind(C, name="ioda_has_variables_c_create_int32")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz_name
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) ::dims
         type(c_ptr) :: var
      end function

      function ioda_has_variables_c_create2_int64(p, sz_name, name, n_dims, dims, max_dims, c_param) &
      & result(var) bind(C, name="ioda_has_variables_c_create2_int64")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr), value :: c_param
         type(c_ptr) :: var
         integer(c_int64_t) :: sz_name
         integer(c_int64_t) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) :: dims
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
      end function

      function ioda_has_variables_c_create_int64(p, sz_name, name, n_dims, dims) result(var) &
      & bind(C, name="ioda_has_variables_c_create_int64")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz_name
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) ::dims
         type(c_ptr) :: var
      end function

      function ioda_has_variables_c_create2_str(p, sz_name, name, n_dims, dims, max_dims, c_param) &
      & result(var) bind(C, name="ioda_has_variables_c_create2_str")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         type(c_ptr), value :: name
         type(c_ptr), value :: c_param
         type(c_ptr) :: var
         integer(c_int64_t) :: sz_name
         integer(c_int64_t) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) :: dims
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
      end function

      function ioda_has_variables_c_create_str(p, sz_name, name, n_dims, dims) result(var) &
      & bind(C, name="ioda_has_variables_c_create_str")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: sz_name
         type(c_ptr), value :: name
         integer(c_int64_t), intent(in) :: n_dims
         integer(c_int64_t), dimension(:), intent(in) ::dims
         type(c_ptr) :: var
      end function

   end interface
contains

   subroutine ioda_has_variables_init(p)
      implicit none
      type(ioda_has_variables) :: p
      p%data_ptr = ioda_has_variables_c_alloc()
   end subroutine

   subroutine ioda_has_variables_dtor(this)
      implicit none
      type(ioda_has_variables) :: this
      call ioda_has_variables_c_dtor(this%data_ptr)
   end subroutine

   subroutine ioda_has_variables_copy(this, rhs)
      implicit none
      class(ioda_has_variables), intent(in) :: rhs
      class(ioda_has_variables), intent(out) :: this
      call ioda_has_variables_c_clone(this%data_ptr, rhs%data_ptr)
   end subroutine

   subroutine ioda_has_variables_list(this, list_string)
      implicit none
      class(ioda_has_variables) :: this
      class(ioda_vecstring) :: list_string
      if (c_associated(list_string%data_ptr)) then
         call ioda_vecstring_dealloc(list_string)
      end if
      list_string%data_ptr = ioda_has_variables_c_list(this%data_ptr)
   end subroutine

   logical function ioda_has_variables_remove(this, n, name) result(r)
      implicit none
      class(ioda_has_variables) :: this
      integer(int64), intent(in) :: n
      character(len=*), intent(in) :: name
      type(c_ptr) :: name_ptr

      name_ptr = ioda_f_string_to_c_dup(name)
      r = ioda_has_variables_c_remove(this%data_ptr, n, name_ptr)
      call ioda_c_free(name_ptr)
   end function

   function ioda_has_variables_exists(this, n, name) result(r)
      implicit none
      class(ioda_has_variables) :: this
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: n
      logical :: r
      type(c_ptr) :: name_ptr

      name_ptr = ioda_f_string_to_c_dup(name)
      r = ioda_has_variables_c_exists(this%data_ptr, n, name_ptr)
      call ioda_c_free(name_ptr)
   end function

   subroutine ioda_has_variables_open(this, n, name, var)
      implicit none
      class(ioda_has_variables) :: this
      character(len=*), intent(in) :: name
      class(ioda_variable) :: var
      integer(int64) :: n
      type(c_ptr) :: name_ptr

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_ptr = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_open(this%data_ptr, n, name_ptr)
      call ioda_c_free(name_ptr)
   end subroutine

   subroutine ioda_has_variables_create_float(this, name, ndim, dims, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_c = ioda_f_string_to_c_dup(name)
      name_sz = len_trim(name, int64)
      var%data_ptr = ioda_has_variables_c_create_float(this%data_ptr, name_sz, name_c, ndim, dims)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create2_float(this, name, ndim, dims, max_dims, cparams, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      class(ioda_variable_creation_parameters), intent(in) :: cparams
      integer(int64), dimension(:), intent(in) :: max_dims
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_sz = len_trim(name, int64)
      name_c = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_create2_float(this%data_ptr, name_sz, name_c, ndim, dims,&
              & max_dims, cparams%data_ptr)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create_double(this, name, ndim, dims, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_c = ioda_f_string_to_c_dup(name)
      name_sz = len_trim(name, int64)
      var%data_ptr = ioda_has_variables_c_create_double(this%data_ptr, name_sz, name_c, ndim, dims)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create2_double(this, name, ndim, dims, max_dims, cparams, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      class(ioda_variable_creation_parameters), intent(in) :: cparams
      integer(int64), dimension(:), intent(in) :: max_dims
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_sz = len_trim(name, int64)
      name_c = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_create2_double(this%data_ptr, name_sz, name_c, ndim, dims,&
              & max_dims, cparams%data_ptr)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create_char(this, name, ndim, dims, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_c = ioda_f_string_to_c_dup(name)
      name_sz = len_trim(name, int64)
      var%data_ptr = ioda_has_variables_c_create_char(this%data_ptr, name_sz, name_c, ndim, dims)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create2_char(this, name, ndim, dims, max_dims, cparams, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      class(ioda_variable_creation_parameters), intent(in) :: cparams
      integer(int64), dimension(:), intent(in) :: max_dims
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_sz = len_trim(name, int64)
      name_c = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_create2_char(this%data_ptr, name_sz, name_c, ndim, dims,&
              & max_dims, cparams%data_ptr)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create_int16(this, name, ndim, dims, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_c = ioda_f_string_to_c_dup(name)
      name_sz = len_trim(name, int64)
      var%data_ptr = ioda_has_variables_c_create_int16(this%data_ptr, name_sz, name_c, ndim, dims)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create2_int16(this, name, ndim, dims, max_dims, cparams, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      class(ioda_variable_creation_parameters), intent(in) :: cparams
      integer(int64), dimension(:), intent(in) :: max_dims
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_sz = len_trim(name, int64)
      name_c = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_create2_int16(this%data_ptr, name_sz, name_c, ndim, dims,&
              & max_dims, cparams%data_ptr)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create_int32(this, name, ndim, dims, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_c = ioda_f_string_to_c_dup(name)
      name_sz = len_trim(name, int64)
      var%data_ptr = ioda_has_variables_c_create_int32(this%data_ptr, name_sz, name_c, ndim, dims)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create2_int32(this, name, ndim, dims, max_dims, cparams, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      class(ioda_variable_creation_parameters), intent(in) :: cparams
      integer(int64), dimension(:), intent(in) :: max_dims
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_sz = len_trim(name, int64)
      name_c = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_create2_int32(this%data_ptr, name_sz, name_c, ndim, dims,&
              & max_dims, cparams%data_ptr)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create_int64(this, name, ndim, dims, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_c = ioda_f_string_to_c_dup(name)
      name_sz = len_trim(name, int64)
      var%data_ptr = ioda_has_variables_c_create_int64(this%data_ptr, name_sz, name_c, ndim, dims)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create2_int64(this, name, ndim, dims, max_dims, cparams, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      class(ioda_variable_creation_parameters), intent(in) :: cparams
      integer(int64), dimension(:), intent(in) :: max_dims
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_sz = len_trim(name, int64)
      name_c = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_create2_int64(this%data_ptr, name_sz, name_c, ndim, dims,&
              & max_dims, cparams%data_ptr)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create_str(this, name, ndim, dims, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_c = ioda_f_string_to_c_dup(name)
      name_sz = len_trim(name, int64)
      var%data_ptr = ioda_has_variables_c_create_str(this%data_ptr, name_sz, name_c, ndim, dims)
      call ioda_c_free(name_c)
   end subroutine

   subroutine ioda_has_variables_create2_str(this, name, ndim, dims, max_dims, cparams, var)
      implicit none
      class(ioda_has_variables), intent(in) :: this
      class(ioda_variable), intent(out) :: var
      character(len=*), intent(in) :: name
      class(ioda_variable_creation_parameters), intent(in) :: cparams
      integer(int64), dimension(:), intent(in) :: max_dims
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: dims
      integer(int64) :: name_sz
      type(c_ptr) :: name_c

      if (c_associated(var%data_ptr)) then
         call ioda_variable_dtor(var)
      end if
      name_sz = len_trim(name, int64)
      name_c = ioda_f_string_to_c_dup(name)
      var%data_ptr = ioda_has_variables_c_create2_str(this%data_ptr, name_sz, name_c, ndim, dims,&
              & max_dims, cparams%data_ptr)
      call ioda_c_free(name_c)
   end subroutine

end module
