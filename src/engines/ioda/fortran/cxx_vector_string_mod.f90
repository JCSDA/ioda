module cxx_vector_string_mod
	use,intrinsic :: iso_c_binding
	use,intrinsic :: iso_fortran_env
	use :: f_c_string_mod
	use :: cxx_string_mod
		
	type :: cxx_vector_string
		type(c_ptr) :: data_ptr = c_null_ptr
	contains
		final cxx_vector_string_dealloc
		procedure :: cxx_vector_string_set
		procedure :: cxx_vector_string_set_str
		generic,public :: set => cxx_vector_string_set,&
			cxx_vector_string_set_str
		procedure :: cxx_vector_string_get
		procedure :: cxx_vector_string_get_str
		generic,public :: get => cxx_vector_string_get,&
			cxx_vector_string_get_str
		procedure,pass(this) :: cxx_vector_string_copy
		generic,public :: assignment(=) => cxx_vector_string_copy
		procedure :: cxx_vector_string_push_back
		procedure :: cxx_vector_string_push_back_str
		generic,public :: push_back => cxx_vector_string_push_back,&
			cxx_vector_string_push_back_str
		procedure :: size => cxx_vector_string_size
		procedure :: element_size => cxx_vector_string_element_size
		procedure :: clear => cxx_vector_string_clear 
		procedure :: empty => cxx_vector_string_empty
		procedure :: associated => cxx_vector_string_associated
		procedure :: resize => cxx_vector_string_resize
	end type
	
	interface
		function cxx_vector_string_c_alloc() result(p) bind(C,name="cxx_vector_string_c_alloc")
			import c_ptr
			type(c_ptr) :: p
		end function
		subroutine cxx_vector_string_c_dealloc(p) bind(C,name="cxx_vector_string_c_dealloc")
			import c_ptr
			type(c_ptr) :: p
		end subroutine	
		subroutine cxx_vector_string_c_copy(p,o) bind(C,name="cxx_vector_string_c_copy")
			import c_ptr
			type(c_ptr) :: p
			type(c_ptr),value :: o
		end subroutine
		subroutine cxx_vector_string_c_set(p,i,v) bind(C,name="cxx_vector_string_c_set")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t),value :: i
			type(c_ptr),value :: v
		end subroutine		
		subroutine cxx_vector_string_c_set_str(p,i,v) bind(C,name="cxx_vector_string_c_set_str")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t),value :: i
			type(c_ptr),value :: v
		end subroutine		
		function cxx_vector_string_c_get(p,i) result(r) bind(C,name="cxx_vector_string_c_get")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t),value :: i
			type(c_ptr) :: r
		end function
		function cxx_vector_string_c_get_str(p,i) result(r) bind(C,name="cxx_vector_string_c_get_str")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t),value :: i
			type(c_ptr) :: r
		end function
		function cxx_vector_string_c_size(p) result(sz) bind(C,name="cxx_vector_string_c_size")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t) :: sz
		end function
		function cxx_vector_string_c_element_size(p,i) result(sz) &
			bind(C,name="cxx_vector_string_c_element_size")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t),value :: i
			integer(c_int64_t) :: 	sz
		end function
		subroutine cxx_vector_string_c_clear(p) bind(C,name="cxx_vector_string_c_clear")
			import C_ptr
			type(c_ptr),value :: p
		end subroutine
		subroutine cxx_vector_string_c_push_back(p,r) bind(C,name="cxx_vector_string_c_push_back")
			import c_ptr
			type(c_ptr),value :: p
			type(c_ptr),value :: r
		end subroutine
		subroutine cxx_vector_string_c_push_back_str(p,r) bind(C,name="cxx_vector_string_c_push_back_str")
			import c_ptr
			type(c_ptr),value :: p
			type(c_ptr),value :: r
		end subroutine
		subroutine cxx_vector_string_c_resize(p,n) bind(C,name="cxx_vector_string_c_resize")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t),value :: n
		end subroutine
		function cxx_vector_string_c_empty(p) result(r) bind(C,name="cxx_vector_string_c_empty")
			import c_ptr,c_int
			type(c_ptr),value :: p
			integer(c_int) :: r
		end function
	end interface
contains
	subroutine cxx_vector_string_init(ptr)
		implicit none
		type(cxx_vector_string),intent(inout) :: ptr
		if ( .not. c_associated(ptr%data_ptr) ) then
			ptr%data_ptr = cxx_vector_string_c_alloc()
		end if
	end subroutine
	subroutine cxx_vector_string_dealloc(this)
		implicit none
		type(cxx_vector_string),intent(inout) :: this
		if ( c_associated(this%data_ptr) ) then
			call cxx_vector_string_c_dealloc(this%data_ptr)
		end if
		this%data_ptr = c_null_ptr
	end subroutine
	subroutine cxx_vector_string_copy(this,other)
		implicit none
		class(cxx_vector_string),intent(inout) :: this
		class(cxx_vector_string),intent(in) :: other

		if ( .not. c_associated(this%data_ptr) ) then
			this%data_ptr = cxx_vector_string_c_alloc()
		end if
		call cxx_vector_string_c_copy(this%data_ptr,other%data_ptr)
	end subroutine
	subroutine cxx_vector_string_set_str(this,i,str)
		implicit none
		class(cxx_vector_string),intent(inout) :: this
		integer(int64),intent(in) :: i
		class(cxx_string),intent(in) :: str
		type(c_ptr) :: cstr_ptr
		integer(int64) :: j
		j = i - 1
		if ( .not. c_associated(this%data_ptr) ) then
			this%data_ptr = cxx_vector_string_c_alloc()
		end if
		call cxx_vector_string_c_set_str(this%data_ptr,j,str%data_ptr)
	end subroutine
	subroutine cxx_vector_string_set(this,i,fstr)
		implicit none
		class(cxx_vector_string),intent(inout) :: this
		integer(int64),intent(in) :: i
		character(len=*),intent(in) :: fstr
		type(c_ptr) :: cstr_ptr
		integer(int64) :: j
		j = i - 1
		if ( .not. c_associated(this%data_ptr) ) then
		end if
		cstr_ptr = f_string_to_c_dup(fstr)
		call cxx_vector_string_c_set(this%data_ptr,j,cstr_ptr)
		call c_free(cstr_ptr)
	end subroutine
	subroutine cxx_vector_string_push_back_str(this,other)
		implicit none
		class(cxx_vector_string),intent(inout) :: this
		class(cxx_string),intent(in) :: other
		call cxx_vector_string_c_push_back_str(this%data_ptr,other%data_ptr)
	end subroutine
	subroutine cxx_vector_string_push_back(this,fstr)
		implicit none
		class(cxx_vector_string),intent(inout) :: this
		character(len=*),intent(in) :: fstr
		type(c_ptr) :: cstr_ptr

		if ( .not. c_associated(this%data_ptr)) then
			this%data_ptr = cxx_vector_string_c_alloc()
		end if
		cstr_ptr = f_string_to_c_dup(fstr);
		call cxx_vector_string_c_push_back(this%data_ptr,cstr_ptr)
		call c_free(cstr_ptr)
	end subroutine
	subroutine cxx_vector_string_get_str(this,i,cxx_str)
		implicit none
		class(cxx_vector_string),intent(in) :: this
		class(cxx_string) :: cxx_str
		integer(int64),intent(in) :: i
		integer(int64) :: j
		j = i - 1
		cxx_str%data_ptr = cxx_vector_string_c_get_str(this%data_ptr,j)
	end subroutine
	subroutine cxx_vector_string_get(this,i,fstr)
		implicit none
		class(cxx_vector_string),intent(in) :: this
		integer(int64),intent(in) :: i
		character(len=*) :: fstr
		type(c_ptr) :: cstr_ptr
		integer(int64) :: j
		j = i - 1
		cstr_ptr = cxx_vector_string_c_get(this%data_ptr,j)
		call c_string_to_f_copy(cstr_ptr,fstr);
		call c_free(cstr_ptr)
	end subroutine
	integer(int64) function cxx_vector_string_size(this) result(sz)
		implicit none
		class(cxx_vector_string),intent(in) :: this
		sz = cxx_vector_string_c_size(this%data_ptr)
	end function
	integer(int64) function cxx_vector_string_element_size(this,i) result(sz)
		implicit none
		class(cxx_vector_string),intent(in) :: this
		integer(int64),intent(in) :: i
		integer(int64) :: j
		j = i - 1
		sz = cxx_vector_string_c_element_size(this%data_ptr,j)
	end function
	subroutine cxx_vector_string_clear(this)
		implicit none
		class(cxx_vector_string) :: this
		call cxx_vector_string_c_clear(this%data_ptr)
	end subroutine
	subroutine cxx_vector_string_resize(this,n)
		implicit none
		class(cxx_vector_string),intent(inout) :: this
		integer(int64),intent(in) :: n
		integer(int64) :: m
		if ( .not. c_associated(this%data_ptr) ) then
			this%data_ptr = cxx_vector_string_c_alloc()
		end if
		write(error_unit,*)' f mod resize ',n
		call cxx_vector_string_c_resize(this%data_ptr,n)
		m = cxx_vector_string_c_size(this%data_ptr)
		write(error_unit,*)' f after resize ',m
	end subroutine
	logical function cxx_vector_string_associated(this) result(r)
		implicit none
		class(cxx_vector_string),intent(in) :: this
		if ( c_associated(this%data_ptr) ) then
			r = .false.
		else
			r = .true.
		end if
	end function
	logical function cxx_vector_string_empty(this) result(r)
		implicit none
		class(cxx_vector_string),intent(in) :: this
		if ( cxx_vector_string_c_empty(this%data_ptr) == 1) then
			r = .true.
		else
			r = .false.
		end if
	end function
end module				