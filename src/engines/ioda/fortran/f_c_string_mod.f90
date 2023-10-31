module f_c_string_mod
	use,intrinsic :: iso_c_binding
	use,intrinsic :: iso_fortran_env
	public
	interface
		function c_alloc(n) result(p) bind(C,name="Malloc")
			import c_ptr,c_int64_t
			type(c_ptr) :: p
			integer(c_int64_t),value :: n
		end function
		subroutine c_free(p) bind(C,name='free')
			import C_ptr
			type(c_ptr),value :: p
		end subroutine
		function c_strlen(p) result(n) bind(C,name='strlen')
			import C_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t) :: n
		end function
		function c_strdup(p) result(pc) bind(C,name='strdup')
			import C_ptr
			type(c_ptr),value :: p
			type(c_ptr) :: pc
		end function
	end interface
contains
	subroutine f_string_to_c_copy(fstr,cstr_ptr,n)
		implicit none
		type(c_ptr),intent(inout) :: cstr_ptr
		character(len=*),intent(in) :: fstr
		integer(int64),intent(inout) :: n
		character(kind=c_char,len=1),dimension(:),pointer :: cstr
		integer(int64) :: fsz,csz,i
		fsz = len_trim(fstr,int64)
		csz = fsz + 1
		if ( fsz > n) then
			if ( c_associated(cstr_ptr) ) then
				call c_free(cstr_ptr)
			end if
			cstr_ptr = c_alloc(csz)
			n = fsz			
		end if
		call c_f_pointer(cstr_ptr,cstr,[csz])
		do i=1,fsz
			cstr(i) = fstr(i:i)
		end do
		cstr(csz) = c_null_char
	end subroutine
	function f_string_to_c_dup(fstr) result(cstr_ptr)
		implicit none
		type(c_ptr) :: cstr_ptr
		character(len=*),intent(in) :: fstr
		character(kind=c_char,len=1),dimension(:),pointer :: cstr
		integer(int64) :: fsz,csz,i
		fsz = len_trim(fstr,int64)
		csz = fsz + 1
		cstr_ptr = c_alloc(csz)
		call c_f_pointer(cstr_ptr,cstr,[csz])
		do i=1,fsz
			cstr(i) = fstr(i:i)
		end do
		cstr(csz) = c_null_char
	end function
	subroutine c_string_to_f_copy(cstr_ptr,fstr)
		implicit none
		type(c_ptr),intent(in) :: cstr_ptr
		character(len=*),intent(inout) :: fstr
		character(kind=c_char,len=1),dimension(:),pointer :: cstr
		integer(int64) :: i,fsz,csz
		fsz = len(fstr,int64)		
		csz = c_strlen(cstr_ptr)
		if ( fsz < csz) then
			write(error_unit,*)' c_string_to_f_copy fortran string too short'
			stop -1
		end if
		call c_f_pointer(cstr_ptr,cstr,[csz])
		do i=1,csz
			fstr(i:i) = cstr(i)
		end do
		do i=csz+1,fsz
			fstr(i:i) = ' '
		end do
	end subroutine
	subroutine c_string_print(cstr_ptr,unit_no)
		implicit none
		type(c_ptr),intent(in) :: cstr_ptr
		integer,intent(in) :: unit_no
		character(kind=c_char,len=1),dimension(:),pointer :: cstr
		integer(int64) :: csz
		csz = c_strlen(cstr_ptr)
		call c_f_pointer(cstr_ptr,cstr,[csz])
		write(unit_no,*)cstr(1:csz)
	end subroutine
end module


