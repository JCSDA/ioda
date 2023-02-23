module ioda_dimensions_mod
   use, intrinsic :: iso_c_binding
   use, intrinsic :: iso_fortran_env

   type :: ioda_dimensions
      type(c_ptr) :: data_ptr
   contains
      final ioda_dimensions_dtor
      procedure :: set => ioda_dimensions_set
      procedure :: get_dims_cur => ioda_dimensions_get_dims_cur
      procedure :: get_dims_max => ioda_dimensions_get_dims_max
      procedure :: get_dims_cur_size => ioda_dimensions_get_dims_cur_size
      procedure :: get_dims_max_size => ioda_dimensions_get_dims_max_size
      procedure :: get_num_elements => ioda_dimensions_get_num_elements
      procedure :: get_dimensionality => ioda_dimensions_get_dimensionality
      procedure, private, pass(this) :: ioda_dimensions_copy
      generic, public :: assignment(=) => ioda_dimensions_copy
   end type

   interface

      function ioda_dimensions_c_alloc() result(p) bind(C, name="ioda_dimensions_c_alloc")
         import c_ptr
         type(c_ptr) :: p
      end function

      subroutine ioda_dimensions_c_dtor(p) bind(C, name="ioda_dimensions_c_dtor")
         import c_ptr
         type(c_ptr) :: p
      end subroutine

      subroutine ioda_dimensions_c_clone(this, rhs) bind(C, name="ioda_dimensions_c_clone")
         import c_ptr
         type(c_ptr), value :: rhs
         type(c_ptr) :: this
      end subroutine

      subroutine ioda_dimensions_c_set(p, ndim, n_curr_dim, n_max_dim, max_dims, cur_dims) bind(C, name="ioda_dimensions_c_set")
         import c_ptr, c_int64_t
         type(c_ptr) :: p
         integer(c_int64_t), value, intent(in) :: ndim, n_curr_dim, n_max_dim
         integer(c_int64_t), dimension(:), intent(in) :: max_dims
         integer(c_int64_t), dimension(:), intent(in) :: cur_dims
      end subroutine

      subroutine ioda_dimensions_c_get_dims_max(p, d, n) bind(C, name="ioda_dimensions_c_get_dims_max")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), intent(out) :: n
         integer(c_int64_t), dimension(:), intent(out) :: d
      end subroutine

      subroutine ioda_dimensions_c_get_dims_cur(p, d, n) bind(C, name="ioda_dimensions_c_get_dims_cur")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t), intent(out) :: n
         integer(c_int64_t), dimension(:), intent(out) :: d
      end subroutine

      function ioda_dimensions_c_get_dimensionality(p) result(n)  &
       & bind(C, name="ioda_dimensions_c_get_dimensionality")
         import c_ptr, c_int64_t
         type(C_ptr), value :: p
         integer(c_int64_t) :: n
      end function

      function ioda_dimensions_c_get_num_elements(p) result(d) bind(C, name="ioda_dimensions_c_num_of_elements")
         import c_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: d
      end function

      function ioda_dimensions_c_get_dims_cur_size(p) result(d) bind(C, name="ioda_dimensions_c_get_dims_cur_size")
         import C_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: d
      end function

      function ioda_dimensions_c_get_dims_max_size(p) result(d) bind(C, name="ioda_dimensions_c_get_dims_max_size")
         import C_ptr, c_int64_t
         type(c_ptr), value :: p
         integer(c_int64_t) :: d
      end function

   end interface

contains
   subroutine ioda_dimensions_init(this)
      implicit none
      type(ioda_dimensions) :: this
      this%data_ptr = ioda_dimensions_c_alloc()
   end subroutine

   subroutine ioda_dimensions_dtor(this)
      implicit none
      type(ioda_dimensions) :: this
      call ioda_dimensions_c_dtor(this%data_ptr)
   end subroutine

   subroutine ioda_dimensions_copy(this, rhs)
      implicit none
      class(ioda_dimensions), intent(in) :: rhs
      class(ioda_dimensions), intent(out) :: this
      call ioda_dimensions_c_clone(this%data_ptr, rhs%data_ptr)
   end subroutine

   subroutine ioda_dimensions_set(this, ndim, n_curr_dim, n_max_dim, dims_max, dims_cur)
      implicit none
      class(ioda_dimensions) :: this
      integer(int64), intent(in) :: ndim, n_curr_dim, n_max_dim
      integer(int64), dimension(:), intent(in) :: dims_max
      integer(int64), dimension(:), intent(in) :: dims_cur
      call ioda_dimensions_c_set(this%data_ptr, ndim, n_curr_dim, n_max_dim, dims_max, dims_cur)
   end subroutine

   subroutine ioda_dimensions_get_dims_cur(this, dims, nd)
      implicit none
      class(ioda_dimensions) :: this
      integer(int64), intent(out) :: nd
      integer(int64), dimension(:), intent(out) :: dims
      call ioda_dimensions_c_get_dims_cur(this%data_ptr, dims, nd)
   end subroutine

   subroutine ioda_dimensions_get_dims_max(this, dims, nd)
      implicit none
      class(ioda_dimensions) :: this
      integer(int64), intent(out) :: nd
      integer(int64), dimension(:), intent(out) :: dims
      call ioda_dimensions_c_get_dims_max(this%data_ptr, dims, nd)
   end subroutine

   integer(int64) function ioda_dimensions_get_dims_max_size(this) result(nd)
      implicit none
      class(ioda_dimensions) :: this
      nd = ioda_dimensions_c_get_dims_max_size(this%data_ptr)
   end function

   function ioda_dimensions_get_dims_cur_size(this) result(nd)
      implicit none
      class(ioda_dimensions) :: this
      integer(int64) :: nd
      nd = ioda_dimensions_c_get_dims_cur_size(this%data_ptr)
   end function

   function ioda_dimensions_get_num_elements(this) result(nd)
      implicit none
      class(ioda_dimensions) :: this
      integer(int64) :: nd
      nd = ioda_dimensions_c_get_num_elements(this%data_ptr)
   end function

   function ioda_dimensions_get_dimensionality(this) result(nd)
      implicit none
      class(ioda_dimensions) :: this
      integer(int64) :: nd
      nd = ioda_dimensions_c_get_dimensionality(this%data_ptr)
   end function

end module
