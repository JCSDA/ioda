module cxx_vector_dbl_mod
    use,intrinsic :: iso_fortran_env, only: int32, int64, real64
    use,intrinsic :: iso_c_binding, only: &
        c_associated, c_double, c_int, c_int64_t, c_null_ptr, c_ptr
 implicit none
    public

    type :: cxx_vector_dbl
        type(c_ptr) :: data_ptr    = c_null_ptr
    contains
        final :: cxx_vector_dbl_dealloc
        procedure,pass(this) :: cxx_vector_dbl_copy
        generic,public :: assignment(=) => cxx_vector_dbl_copy
        procedure :: push_back => cxx_vector_dbl_push_back
        procedure :: set => cxx_vector_dbl_set
        procedure :: get => cxx_vector_dbl_get
        procedure :: resize => cxx_vector_dbl_resize
        procedure :: size => cxx_vector_dbl_size
        procedure :: clear => cxx_vector_dbl_clear
        procedure :: empty => cxx_vector_dbl_empty
        procedure :: associated => cxx_vector_dbl_associated
    end type cxx_vector_dbl

    interface
        subroutine cxx_vector_dbl_c_dealloc(p) bind(C,name="cxx_vector_dbl_c_dealloc")
            import c_ptr
            implicit none
            type(c_ptr), intent(inout) :: p
        end subroutine cxx_vector_dbl_c_dealloc
        function cxx_vector_dbl_c_alloc() result(p) bind(C,name="cxx_vector_dbl_c_alloc")
            import c_ptr
            implicit none
            type(c_ptr), intent(inout) :: p
        end function cxx_vector_dbl_c_alloc
        subroutine cxx_vector_dbl_c_copy(p,q) bind(C,name="cxx_vector_dbl_c_copy")
            import C_ptr
            implicit none
            type(c_ptr), intent(inout) :: p
            type(c_ptr), value :: q
        end subroutine cxx_vector_dbl_c_copy
        subroutine cxx_vector_dbl_c_push_back(p,x) bind(C,name="cxx_vector_dbl_c_push_back")
            import c_ptr,c_double
            implicit none
            type(C_ptr), value :: p
            real(c_double), value :: x
        end subroutine cxx_vector_dbl_c_push_back
        subroutine cxx_vector_dbl_c_set(p,i,x) bind(C,name="cxx_vector_dbl_c_set")
            import c_ptr,c_int64_t,c_double
            implicit none
            type(C_ptr), value :: p
            integer(c_int64_t), value :: i
            real(c_double), value :: x
        end subroutine cxx_vector_dbl_c_set
        function cxx_vector_dbl_c_get(p,i) result(vi)  bind(C,name="cxx_vector_dbl_c_get")
            import c_ptr,c_int64_t,c_double
            implicit none
            type(c_ptr), value :: p
            integer(c_int64_t), value :: i
            real(c_double) :: vi
        end function cxx_vector_dbl_c_get
        subroutine cxx_vector_dbl_c_resize(p,n) bind(C,name="cxx_vector_dbl_c_resize")
            import c_ptr,c_int64_t
            implicit none
            type(c_ptr), value :: p
            integer(c_int64_t), value :: n
        end subroutine cxx_vector_dbl_c_resize
        subroutine cxx_vector_dbl_c_clear(p) bind(C,name="cxx_vector_dbl_c_clear")
            import c_ptr
            implicit none
            type(c_ptr), value :: p
         end subroutine cxx_vector_dbl_c_clear
        function cxx_vector_dbl_c_size(p) result(sz) bind(C,name="cxx_vector_dbl_c_size")
            import c_ptr,c_int64_t
            implicit none
            type(c_ptr), value :: p
            integer(c_int64_t) :: sz
        end function cxx_vector_dbl_c_size
        function cxx_vector_dbl_c_empty(p) result(e) bind(C,name="cxx_vector_dbl_c_empty")
            import c_ptr,c_int
            implicit none
            type(c_ptr), value :: p
            integer(c_int) :: e
        end function cxx_vector_dbl_c_empty
     end interface
contains
    subroutine cxx_vector_dbl_init(p)
        implicit none
        type(cxx_vector_dbl),intent(inout) :: p
        if ( .not. c_associated(p%data_ptr) ) then
            p%data_ptr = cxx_vector_dbl_c_alloc()
        end if
    end subroutine cxx_vector_dbl_init
    subroutine cxx_vector_dbl_dealloc(this)
        implicit none
        type(cxx_vector_dbl), intent(inout) :: this
        if ( c_associated(this%data_ptr) ) then
            call cxx_vector_dbl_c_dealloc(this%data_ptr)
        end if
    end subroutine cxx_vector_dbl_dealloc
    subroutine cxx_vector_dbl_copy(this,other)
        implicit none
        class(cxx_vector_dbl),intent(inout) :: this
        class(cxx_vector_dbl),intent(in) :: other
        if ( .not. c_associated(this%data_ptr) ) then
            this%data_ptr = cxx_vector_dbl_c_alloc()
        end if
        call cxx_vector_dbl_c_copy(this%data_ptr,other%data_ptr)
    end subroutine cxx_vector_dbl_copy
    subroutine cxx_vector_dbl_push_back(this,x)
        implicit none
        class(cxx_vector_dbl),intent(inout) :: this
        real(real64),intent(in) :: x
        call cxx_vector_dbl_c_push_back(this%data_ptr,x)
    end subroutine cxx_vector_dbl_push_back
    subroutine cxx_vector_dbl_set(this,i,x)
        implicit none
        class(cxx_vector_dbl),intent(inout) :: this
        integer(int64),intent(in) :: i
        real(real64),intent(in) :: x
        call cxx_vector_dbl_c_set(this%data_ptr,i,x)
    end subroutine cxx_vector_dbl_set
    function cxx_vector_dbl_get(this,i) result(vi)
        implicit none
        class(cxx_vector_dbl),intent(in) :: this
        integer(int64),intent(in) :: i
        real(real64) :: vi
        vi = cxx_vector_dbl_c_get(this%data_ptr,i)
    end function cxx_vector_dbl_get
    function cxx_vector_dbl_size(this) result(sz)
        implicit none
        class(cxx_vector_dbl),intent(in) :: this
        integer(int64) :: sz
        sz = cxx_vector_dbl_c_size(this%data_ptr)
    end function cxx_vector_dbl_size
    logical function cxx_vector_dbl_empty(this) result(r)
        implicit none
        class(cxx_vector_dbl),intent(in) :: this
        integer(int32) :: i
        r = .false.
        i = cxx_vector_dbl_c_empty(this%data_ptr)
        if ( i == 1 ) r = .true.
    end function cxx_vector_dbl_empty
    logical function cxx_vector_dbl_associated(this) result(r)
        implicit none
        class(cxx_vector_dbl),intent(in) :: this
        if ( c_associated(this%data_ptr) ) then
            r = .true.
        else
            r = .false.
        end if
    end function cxx_vector_dbl_associated
    subroutine cxx_vector_dbl_resize(this,n)
        implicit none
        class(cxx_vector_dbl), intent(inout):: this
        integer(int64), intent(in) :: n
        call cxx_vector_dbl_c_resize(this%data_ptr,n)
    end subroutine cxx_vector_dbl_resize
    subroutine cxx_vector_dbl_clear(this)
        implicit none
        class(cxx_vector_dbl), intent(inout) :: this
        call cxx_vector_dbl_c_clear(this%data_ptr)
    end subroutine cxx_vector_dbl_clear
end module cxx_vector_dbl_mod
