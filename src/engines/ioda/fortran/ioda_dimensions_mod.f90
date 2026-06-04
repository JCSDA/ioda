!
! (C) Copyright 2023 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module ioda_dimensions_mod
    use, intrinsic :: iso_c_binding, only: c_int64_t, c_null_ptr, c_ptr
    use, intrinsic :: iso_fortran_env, only: int64
    implicit none
    private

    type, public :: ioda_dimensions
    type(c_ptr) :: data_ptr = c_null_ptr
    contains
        final :: ioda_dimensions_dtor
        procedure :: set => ioda_dimensions_set
        procedure :: get_dims_cur => ioda_dimensions_get_dims_cur
        procedure :: get_dims_max => ioda_dimensions_get_dims_max
        procedure :: get_dims_cur_size => ioda_dimensions_get_dims_cur_size
        procedure :: get_dims_max_size => ioda_dimensions_get_dims_max_size
        procedure :: get_num_elements => ioda_dimensions_get_num_elements
        procedure :: get_dimensionality => ioda_dimensions_get_dimensionality
            procedure,private,pass(this) :: ioda_dimensions_copy
    generic, public :: assignment(=) => ioda_dimensions_copy
    end type ioda_dimensions

interface

    function ioda_dimensions_c_alloc() result(p) bind(C,name="ioda_dimensions_c_alloc")
        import c_ptr
        implicit none
        type(c_ptr) :: p
    end function ioda_dimensions_c_alloc

    subroutine ioda_dimensions_c_dtor(p) bind(C,name="ioda_dimensions_c_dtor")
        import c_ptr
        implicit none
        type(c_ptr), intent(inout) :: p
    end subroutine ioda_dimensions_c_dtor

    subroutine ioda_dimensions_c_clone(this,rhs) bind(C,name="ioda_dimensions_c_clone")
        import c_ptr
        implicit none
        type(c_ptr), value :: rhs
        type(c_ptr), intent(inout) :: this
    end subroutine ioda_dimensions_c_clone

    subroutine ioda_dimensions_c_set(p,ndim,n_curr_dim,n_max_dim,max_dims,cur_dims) bind(C,name="ioda_dimensions_c_set")
        import c_ptr,c_int64_t
        implicit none
        type(c_ptr), intent(inout) :: p
        integer(c_int64_t),value,intent(in) :: ndim,n_curr_dim,n_max_dim
        integer(c_int64_t),dimension(*),intent(in) :: max_dims
        integer(c_int64_t),dimension(*),intent(in) :: cur_dims
    end subroutine ioda_dimensions_c_set

    subroutine ioda_dimensions_c_get_dims_max(p,d,n) bind(C,name="ioda_dimensions_c_get_dims_max")
        import c_ptr,c_int64_t
        implicit none
        type(c_ptr), value :: p
        integer(c_int64_t),intent(out) :: n
        integer(c_int64_t),dimension(*),intent(out) :: d
    end subroutine ioda_dimensions_c_get_dims_max

    subroutine ioda_dimensions_c_get_dims_cur(p,d,n) bind(C,name="ioda_dimensions_c_get_dims_cur")
        import c_ptr,c_int64_t
        implicit none
        type(c_ptr), value :: p
        integer(c_int64_t),intent(out) :: n
        integer(c_int64_t),dimension(*),intent(out) :: d
    end subroutine ioda_dimensions_c_get_dims_cur

    function ioda_dimensions_c_get_dimensionality(p) result(n)  &
     & bind(C,name="ioda_dimensions_c_get_dimensionality")
        import c_ptr,c_int64_t
        implicit none
        type(C_ptr), value :: p
        integer(c_int64_t) :: n
    end function ioda_dimensions_c_get_dimensionality

    function ioda_dimensions_c_get_num_elements(p) result(d) bind(C,name="ioda_dimensions_c_num_of_elements")
        import c_ptr,c_int64_t
        implicit none
        type(c_ptr), value :: p
        integer(c_int64_t) :: d
    end function ioda_dimensions_c_get_num_elements

    function ioda_dimensions_c_get_dims_cur_size(p) result(d) bind(C,name="ioda_dimensions_c_get_dims_cur_size")
        import C_ptr,c_int64_t
        implicit none
        type(c_ptr), value :: p
        integer(c_int64_t) :: d
    end function ioda_dimensions_c_get_dims_cur_size

    function ioda_dimensions_c_get_dims_max_size(p) result(d) bind(C,name="ioda_dimensions_c_get_dims_max_size")
        import C_ptr,c_int64_t
        implicit none
        type(c_ptr), value :: p
        integer(c_int64_t) :: d
    end function ioda_dimensions_c_get_dims_max_size

end interface

contains
    subroutine ioda_dimensions_init(this)
        implicit none
        type(ioda_dimensions), intent(inout) :: this
        this%data_ptr = ioda_dimensions_c_alloc()
    end subroutine ioda_dimensions_init

    subroutine ioda_dimensions_dtor(this)
        implicit none
        type(ioda_dimensions), intent(inout) :: this
        call ioda_dimensions_c_dtor(this%data_ptr)
    end subroutine ioda_dimensions_dtor

    subroutine ioda_dimensions_copy(this,rhs)
        implicit none
        class(ioda_dimensions),intent(in) :: rhs
        class(ioda_dimensions),intent(out) :: this
        call ioda_dimensions_c_clone(this%data_ptr,rhs%data_ptr)
    end subroutine ioda_dimensions_copy

    subroutine ioda_dimensions_set(this,ndim,n_curr_dim,n_max_dim,dims_max,dims_cur)
        implicit none
        class(ioda_dimensions), intent(inout) :: this
        integer(int64),intent(in) :: ndim,n_curr_dim,n_max_dim
        integer(int64),dimension(:),intent(in) :: dims_max
        integer(int64),dimension(:),intent(in) :: dims_cur
        call ioda_dimensions_c_set(this%data_ptr,ndim,n_curr_dim,n_max_dim,dims_max,dims_cur)
    end subroutine ioda_dimensions_set

    subroutine ioda_dimensions_get_dims_cur(this,dims,nd)
        implicit none
        class(ioda_dimensions), intent(inout) :: this
        integer(int64),intent(out) :: nd
        integer(int64),dimension(:),intent(out) :: dims
        call ioda_dimensions_c_get_dims_cur(this%data_ptr,dims,nd)
    end subroutine ioda_dimensions_get_dims_cur

    subroutine ioda_dimensions_get_dims_max(this,dims,nd)
        implicit none
        class(ioda_dimensions), intent(inout) :: this
        integer(int64),intent(out) :: nd
        integer(int64),dimension(:),intent(out) :: dims
        call ioda_dimensions_c_get_dims_max(this%data_ptr,dims,nd)
    end subroutine ioda_dimensions_get_dims_max

    integer(int64) function ioda_dimensions_get_dims_max_size(this) result(nd)
        implicit none
        class(ioda_dimensions), intent(inout) :: this
        nd = ioda_dimensions_c_get_dims_max_size(this%data_ptr)
    end function ioda_dimensions_get_dims_max_size

    function ioda_dimensions_get_dims_cur_size(this) result(nd)
        implicit none
        class(ioda_dimensions), intent(inout) :: this
        integer(int64) :: nd
        nd = ioda_dimensions_c_get_dims_cur_size(this%data_ptr)
    end function ioda_dimensions_get_dims_cur_size

    function ioda_dimensions_get_num_elements(this) result(nd)
        implicit none
        class(ioda_dimensions), intent(inout) :: this
        integer(int64) :: nd
        nd = ioda_dimensions_c_get_num_elements(this%data_ptr)
    end function ioda_dimensions_get_num_elements

    function ioda_dimensions_get_dimensionality(this) result(nd)
        implicit none
        class(ioda_dimensions), intent(inout) :: this
        integer(int64) :: nd
        nd = ioda_dimensions_c_get_dimensionality(this%data_ptr)
    end function ioda_dimensions_get_dimensionality

end module ioda_dimensions_mod
