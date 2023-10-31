program string_test
	use,intrinsic :: iso_c_binding
	use,intrinsic :: iso_fortran_env
	use :: f_c_string_mod
	use :: cxx_string_mod
	implicit none
	type(cxx_string) :: s1
	type(cxx_string) :: s2
	type(cxx_string) :: s3
	character(len=129) :: test1
	character(len=129) :: test2
	character(len=129) :: test3
	character(len=129) :: test4	
	integer(int64) :: sz

	test1='this is test string'
	test2='this is another test string'
	call s1%set(test1)
	sz = s1%size()
	if ( sz /= len_trim(test1) ) then
		write(error_unit,*)'set failed'
		write(error_unit,*)' arg1 size = ',sz
		write(error_unit,*)' f size = ',len_trim(test1)
		stop -1
	end if
	call s1%get(test3)
	if ( len_trim(test3) /= len_trim(test1)) then
		write(error_unit,*)'get failed'
		stop -1	
	end if
	s2 = s1
	if ( s2%empty() ) then
		write(error_unit,*)'copy failed data_ptr is null'
		stop -1
	end if
	call s2%get(test4)
	if ( len_trim(test4) /= len_trim(test1)) then
		write(error_unit,*)'copy failed'
		stop -1	
	end if
	call s1%set(test2)
	sz = s1%size()
	if ( sz /= len_trim(test2) ) then
		write(error_unit,*)'set 2 failed'
		stop -1
	end if		
	call s1%clear()
	sz = s1%size()
	if ( sz /= 0) then
		write(error_unit,*)'clear failed'
		stop -1		
	end if
end program