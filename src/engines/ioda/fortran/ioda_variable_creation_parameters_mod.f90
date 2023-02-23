module ioda_variable_creation_parameters_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env

   type :: ioda_variable_creation_parameters
      type(c_ptr) :: data_ptr
   contains
      final ioda_variable_creation_parameters_dtor
      procedure :: chunking => ioda_variable_creation_parameters_chunking
      procedure :: no_compress => ioda_variable_creation_parameters_no_compress
      procedure :: compress_with_szip => ioda_variable_creation_parameters_compress_with_szip
      procedure :: compress_with_gszip => ioda_variable_creation_parameters_compress_with_gzip
      procedure :: set_fill_value_float => ioda_variable_creation_parameters_set_fill_value_float
      procedure :: set_fill_value_double => ioda_variable_creation_parameters_set_fill_value_double
      procedure :: set_fill_value_char => ioda_variable_creation_parameters_set_fill_value_char
      procedure :: set_fill_value_int16 => ioda_variable_creation_parameters_set_fill_value_int16
      procedure :: set_fill_value_int32 => ioda_variable_creation_parameters_set_fill_value_int32
      procedure :: set_fill_value_int64 => ioda_variable_creation_parameters_set_fill_value_int64

      generic :: set_fill_value => set_fill_value_float, set_fill_value_double, set_fill_value_char,&
              & set_fill_value_int16, set_fill_value_int32, set_fill_value_int64

      procedure, private, pass(this) ::ioda_variable_creation_parameters_copy
      generic, public :: assignment(=) => ioda_variable_creation_parameters_copy

   end type

   interface
      function ioda_variable_creation_parameters_c_alloc() result(p) &
              & bind(C, name="ioda_variable_creation_parameters_c_alloc")
         import c_ptr
         type(c_ptr) :: p
      end function

      subroutine ioda_variable_creation_parameters_c_dtor(p) &
              & bind(C, name="ioda_variable_creation_parameters_c_dtor")
         import c_ptr
         type(c_ptr), value :: p
      end subroutine

      subroutine ioda_variable_creation_parameters_c_clone(this, rhs) &
              & bind(C, name="ioda_variable_creation_parameters_c_clone")
         import c_ptr
         type(c_ptr), value :: rhs
         type(c_ptr) :: this
      end subroutine

      subroutine ioda_variable_creation_parameters_c_no_compress(p) &
              & bind(C, name="ioda_variable_creation_parameters_c_no_compress")
         import c_ptr
         type(c_ptr), value :: p
      end subroutine

      subroutine ioda_variable_creation_parameters_c_chunking(p, do_chunk, ndim, chunks) &
          & bind(C, name="ioda_variable_creation_parameters_c_chunking")
         import c_ptr, c_int64_t, c_bool
         type(c_ptr), value :: p
         logical(c_bool), value :: do_chunk
         integer(c_int64_t), value :: ndim
         integer(c_int64_t), dimension(:), intent(in) :: chunks
      end subroutine

      subroutine ioda_variable_creation_parameters_c_compress_with_gzip(p, level) &
         & bind(C, name="ioda_variable_creation_parameters_c_compress_with_gzip")
         import c_ptr, c_int32_t
         type(c_ptr), value :: p
         integer(c_int32_t), value :: level
      end subroutine

      subroutine ioda_variable_creation_parameters_c_compress_with_szip(p, pix_per_block, option) &
         & bind(C, name="ioda_variable_creation_parameters_c_compress_with_szip")
         import c_ptr, c_int32_t
         type(c_ptr), value :: p
         integer(c_int32_t), value :: pix_per_block, option
      end subroutine

      subroutine ioda_variable_creation_parameters_c_set_fill_value_float(p, v) &
              & bind(C, name="ioda_variable_creation_parameters_c_set_fill_value_float")
         import c_ptr, c_float
         type(c_ptr), value :: p
         real(c_float) :: v
      end subroutine

      subroutine ioda_variable_creation_parameters_c_set_fill_value_double(p, v) &
              & bind(C, name="ioda_variable_creation_parameters_c_set_fill_value_double")
         import c_ptr, c_double
         type(c_ptr), value :: p
         real(c_double) :: v
      end subroutine

      subroutine ioda_variable_creation_parameters_c_set_fill_value_char(p, v) &
              & bind(C, name="ioda_variable_creation_parameters_c_set_fill_value_char")
         import c_ptr, c_char
         type(c_ptr), value :: p
         character(c_char) :: v
      end subroutine

      subroutine ioda_variable_creation_parameters_c_set_fill_value_int16(p, v) &
              & bind(C, name="ioda_variable_creation_parameters_c_set_fill_value_int16")
         import c_ptr, c_int16_t
         type(c_ptr), value :: p
         integer(c_int16_t) :: v
      end subroutine

      subroutine ioda_variable_creation_parameters_c_set_fill_value_int32(p, v) &
              & bind(C, name="ioda_variable_creation_parameters_c_set_fill_value_int32")
         import c_ptr, c_int32_t
         type(c_ptr), value :: p
         integer(c_int32_t) :: v
      end subroutine

      subroutine ioda_variable_creation_parameters_c_set_fill_value_int64(p, v) &
              & bind(C, name="ioda_variable_creation_parameters_c_set_fill_value_int64")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: v
      end subroutine

   end interface
contains
   subroutine ioda_variable_creation_parameters_init(this)
      implicit none
      type(ioda_variable_creation_parameters) :: this
      this%data_ptr = ioda_variable_creation_parameters_c_alloc()
   end subroutine

   subroutine ioda_variable_creation_parameters_dtor(this)
      implicit none
      type(ioda_variable_creation_parameters) :: this
      call ioda_variable_creation_parameters_c_dtor(this%data_ptr)
   end subroutine

   subroutine ioda_variable_creation_parameters_copy(this, rhs)
      implicit none
      class(ioda_variable_creation_parameters), intent(in) :: rhs
      class(ioda_variable_creation_parameters), intent(out) :: this
      call ioda_variable_creation_parameters_c_clone(this%data_ptr, rhs%data_ptr)
   end subroutine

   subroutine ioda_variable_creation_parameters_no_compress(this)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      call ioda_variable_creation_parameters_c_no_compress(this%data_ptr)
   end subroutine

   subroutine ioda_variable_creation_parameters_compress_with_gzip(this, level)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      integer(int32), intent(in) :: level
      call ioda_variable_creation_parameters_c_compress_with_gzip(this%data_ptr, level)
   end subroutine

   subroutine ioda_variable_creation_parameters_compress_with_szip(this, pix_per_block, option)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      integer(int32), intent(in) :: pix_per_block, option
      call ioda_variable_creation_parameters_c_compress_with_szip(this%data_ptr, pix_per_block, option)
   end subroutine

   subroutine ioda_variable_creation_parameters_chunking(this, do_chunk, ndim, chunks)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      logical, intent(in) :: do_chunk
      integer(int64), intent(in) :: ndim
      integer(int64), dimension(:), intent(in) :: chunks
      logical(c_bool) :: do_chunk_c
      do_chunk_c = do_chunk
      call ioda_variable_creation_parameters_c_chunking(this%data_ptr, do_chunk_c, ndim, chunks)
   end subroutine

   subroutine ioda_variable_creation_parameters_set_fill_value_float(this, val)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      real(real32), intent(in) :: val
      call ioda_variable_creation_parameters_c_set_fill_value_float(this%data_ptr, val)
   end subroutine

   subroutine ioda_variable_creation_parameters_set_fill_value_double(this, val)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      real(real64), intent(in) :: val
      call ioda_variable_creation_parameters_c_set_fill_value_double(this%data_ptr, val)
   end subroutine

   subroutine ioda_variable_creation_parameters_set_fill_value_char(this, val)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      character, intent(in) :: val
      character :: val_c
      val_c = val
      call ioda_variable_creation_parameters_c_set_fill_value_char(this%data_ptr, val_C)
   end subroutine

   subroutine ioda_variable_creation_parameters_set_fill_value_int16(this, val)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      integer(int16), intent(in) :: val
      call ioda_variable_creation_parameters_c_set_fill_value_int16(this%data_ptr, val)
   end subroutine

   subroutine ioda_variable_creation_parameters_set_fill_value_int32(this, val)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      integer(int32), intent(in) :: val
      call ioda_variable_creation_parameters_c_set_fill_value_int32(this%data_ptr, val)
   end subroutine

   subroutine ioda_variable_creation_parameters_set_fill_value_int64(this, val)
      implicit none
      class(ioda_variable_creation_parameters) :: this
      integer(int64), intent(in) :: val
      call ioda_variable_creation_parameters_c_set_fill_value_int64(this%data_ptr, val)
   end subroutine

end module
