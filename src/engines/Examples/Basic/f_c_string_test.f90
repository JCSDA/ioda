program f_c_string_test
	use,intrinsic :: iso_c_binding
	use,intrinsic :: iso_fortran_env
	use :: f_c_string_mod
	implicit none
	type(c_ptr) :: s1 = c_null_ptr
	type(c_ptr) :: s2 = c_null_ptr
	type(c_ptr) :: s3 = c_null_ptr
	character(len=129) :: test1
	character(len=129) :: test2
	character(len=129) :: test3
	character(len=129) :: test4	
	integer(int64) :: sz

	test1='this is test string'
	test2='this is another test string'
	sz = 0	
	call f_string_to_c_copy(test1,s1,sz)
	call c_string_to_f_copy(s1,test4)
	if ( len_trim(test4) /= len_trim(test1) ) then
		write(error_unit,*)'copy-copy failed'
		stop -1
	end if
	s2 = f_string_to_c_dup(test1)
	call c_string_to_f_copy(s2,test3)
	if ( len_trim(test3) /= len_trim(test1) ) then
		write(error_unit,*)'dup-copy failed'
		stop -1
	end if
	sz = 32
	call c_free(s1)
	s1 = c_alloc(sz)
	call f_string_to_c_copy(test1,s1,sz)
	call c_string_to_f_copy(s1,test4)
	if ( len_trim(test4) /= len_trim(test1) ) then
		write(error_unit,*)'copy-copy failed'
		stop -1
	end if
	call c_string_print(s2,error_unit)
	call c_string_print(s1,error_unit)
	call c_free(s1)
	call c_free(s2)
	write(error_unit,*)trim(test1)
end program