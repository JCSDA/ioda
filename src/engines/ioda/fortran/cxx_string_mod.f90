module cxx_string_mod
    use,intrinsic :: iso_c_binding, only: &
        c_associated, c_int64_t, c_null_ptr, c_ptr
    use,intrinsic :: iso_fortran_env, only: int64
    use f_c_string_mod, only: c_free, c_string_to_f_copy, f_string_to_c_dup
    implicit none
    private

    type, public :: cxx_string
        type(c_ptr) :: data_ptr = c_null_ptr
    contains
        final :: cxx_string_dealloc
        procedure :: set => cxx_string_set
        procedure :: get => cxx_string_get
        procedure,pass(this) :: cxx_string_copy
        generic,public :: assignment(=) => cxx_string_copy
        procedure :: cxx_string_append
        procedure :: cxx_string_append_str
        generic,public :: append => cxx_string_append,cxx_string_append_str
        procedure :: size => cxx_string_size
        procedure :: clear => cxx_string_clear
        procedure :: empty => cxx_string_empty
    end type cxx_string

    interface
        function cxx_string_c_alloc() result(p) bind(C,name="cxx_string_c_alloc")
            import c_ptr
            implicit none
            type(c_ptr) :: p
        end function cxx_string_c_alloc
        subroutine cxx_string_c_dealloc(p) bind(C,name="cxx_string_c_dealloc")
            import c_ptr
            implicit none
            type(c_ptr), intent(inout) :: p
        end subroutine cxx_string_c_dealloc
        subroutine cxx_string_c_set(p,v) bind(C,name="cxx_string_c_set")
            import c_ptr
            implicit none
            type(c_ptr), intent(inout) :: p
            type(c_ptr), value :: v
        end subroutine cxx_string_c_set
        subroutine cxx_string_c_copy(p,o) bind(C,name="cxx_string_c_copy")
            import c_ptr
            implicit none
            type(c_ptr), intent(inout) :: p
            type(c_ptr), value :: o
        end subroutine cxx_string_c_copy
        function cxx_string_c_get(p) result(r) bind(C,name="cxx_string_c_get")
            import c_ptr
            implicit none
            type(c_ptr), value :: p
            type(c_ptr) :: r
        end function cxx_string_c_get
        function cxx_string_c_add(p,o) result(r) bind(C,name="cxx_string_c_add")
            import c_ptr
            implicit none
            type(c_ptr), value :: p
            type(c_ptr), value :: o
            type(c_ptr) :: r
        end function cxx_string_c_add
        function cxx_string_c_size(p) result(sz) bind(C,name="cxx_string_c_size")
            import c_ptr,c_int64_t
            implicit none
            type(c_ptr), value :: p
            integer(c_int64_t) :: sz
        end function cxx_string_c_size
        subroutine cxx_string_c_clear(p) bind(C,name="cxx_string_c_clear")
            import c_ptr
            implicit none
            type(c_ptr), value :: p
        end subroutine cxx_string_c_clear
        subroutine cxx_string_c_append(p,r) bind(C,name="cxx_string_c_append")
            import c_ptr
            implicit none
            type(c_ptr), value :: p
            type(c_ptr), value :: r
        end subroutine cxx_string_c_append
        subroutine cxx_string_c_append_str(p,r) bind(C,name="cxx_string_c_append_str")
            import c_ptr
            implicit none
            type(c_ptr), value :: p
            type(c_ptr), value :: r
        end subroutine cxx_string_c_append_str
    end interface
contains
    subroutine cxx_string_init(ptr)
        implicit none
        type(cxx_string),intent(inout) :: ptr
        if ( .not. c_associated(ptr%data_ptr) ) then
            ptr%data_ptr = cxx_string_c_alloc()
        end if
    end subroutine cxx_string_init
    subroutine cxx_string_dealloc(this)
        implicit none
        type(cxx_string), intent(inout) :: this
        if ( c_associated(this%data_ptr) ) then
            call cxx_string_c_dealloc(this%data_ptr)
        end if
        this%data_ptr = c_null_ptr
    end subroutine cxx_string_dealloc
    subroutine cxx_string_copy(this,other)
        implicit none
        class(cxx_string),intent(inout) :: this
        class(cxx_string),intent(in) :: other

        if ( .not. c_associated(this%data_ptr) ) then
            call cxx_string_init(this)
        end if
        call cxx_string_c_copy(this%data_ptr,other%data_ptr)
    end subroutine cxx_string_copy
    subroutine cxx_string_set(this,fstr)
        implicit none
        class(cxx_string), intent(inout) :: this
        character(len=*),intent(in) :: fstr
        type(c_ptr) :: cstr_ptr

        if ( .not. c_associated(this%data_ptr) ) then
            call cxx_string_init(this)
        end if
        cstr_ptr = f_string_to_c_dup(fstr)
        call cxx_string_c_set(this%data_ptr,cstr_ptr)
        call c_free(cstr_ptr)
    end subroutine cxx_string_set
    logical function cxx_string_empty(this) result(r)
        implicit none
        class(cxx_string),intent(in) :: this
        if ( c_associated(this%data_ptr) ) then
            r = .false.
        else
            r = .true.
        end if
    end function cxx_string_empty
    subroutine cxx_string_append(this,other)
        implicit none
        class(cxx_string),intent(inout) :: this
        class(cxx_string),intent(in) :: other
        call cxx_string_c_append_str(this%data_ptr,other%data_ptr)
    end subroutine cxx_string_append
    subroutine cxx_string_append_str(this,fstr)
        implicit none
        class(cxx_string),intent(inout) :: this
        character(len=*),intent(in) :: fstr
        type(c_ptr) :: cstr_ptr
        cstr_ptr = f_string_to_c_dup(fstr)
        call cxx_string_c_append_str(this%data_ptr,cstr_ptr)
        call c_free(cstr_ptr)
    end subroutine cxx_string_append_str
    subroutine cxx_string_get(this,fstr)
        implicit none
        class(cxx_string), intent(inout) :: this
        character(len=*), intent(inout) :: fstr
        type(c_ptr) :: cstr_ptr
        cstr_ptr = cxx_string_c_get(this%data_ptr)
        call c_string_to_f_copy(cstr_ptr,fstr)
        call c_free(cstr_ptr)
    end subroutine cxx_string_get
    integer(int64) function cxx_string_size(this) result(sz)
        implicit none
        class(cxx_string),intent(in) :: this
        sz = cxx_string_c_size(this%data_ptr)
    end function cxx_string_size
    subroutine cxx_string_clear(this)
        implicit none
        class(cxx_string), intent(inout) :: this
        call cxx_string_c_clear(this%data_ptr)
    end subroutine cxx_string_clear
end module cxx_string_mod
