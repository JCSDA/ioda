!
! (C) Copyright 2021-2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Test the ioda Fortran string interface.

program ioda_fortran_string_test
	use,intrinsic :: iso_c_binding
	use,intrinsic :: iso_fortran_env
        use :: ioda_f_c_string_mod
        	
	character(len=256) :: fstr1
	character(len=256) :: fstr2
	character(len=:),allocatable :: fstra	
	type(c_ptr) :: cstr1
	type(c_ptr) :: cstr2
	integer(int64) :: i
	integer(int64) :: szz
		
	fstr1 = 'this is my test string'
	cstr1 = ioda_f_string_to_c_dup(fstr1)
	if (len_trim(fstr1) /= c_strlen(cstr1)) then
	    write(error_unit,*)'ioda_f_string_to_c_dup failed'
	    write(error_unit,*)' f strlen = ',len_trim(fstr1),'>'
	    write(error_unit,*)' c strlen = ',c_strlen(cstr1),'>'
	    write(error_unit,*)' f string = ',trim(fstr1),'>'
	    write(error_unit,*)' c string = '
	    call ioda_c_string_print(cstr1)
	    stop -1
	end if
	call ioda_c_string_to_f_copy(cstr1,fstr2)
	if (len_trim(fstr1) /= len_trim(fstr2)) then
	    write(error_unit,*)'ioda_c_string_to_f_copy failed'
	    stop -1
	end if
	
	do i=1,len(fstr1)
		if ( fstr1(i:i) /= fstr2(i:i) ) then
			write(error_unit,*)'ioda_c_string_to_f_copy failed?'
			write(error_unit,*)' orig ',trim(fstr1)
			write(error_unit,*)' copy ',trim(fstr2)
			stop -1
		end if	
	end do

	fstr1 = 'this is my test string'
	szz = len_trim(fstr1,int64)
	cstr2 = ioda_c_string_alloc(szz)
	call ioda_f_string_to_c_copy(fstr1,cstr2,szz)
	if (len_trim(fstr1) /= c_strlen(cstr2)) then
	    write(error_unit,*)'ioda_f_string_to_c_copy failed'
	    stop -1
	end if
	call ioda_c_string_to_f_dup(cstr2,fstra)
	if (len_trim(fstr1) /= len_trim(fstr2)) then
	    write(error_unit,*)'ioda_c_string_to_f_dup failed'
	    stop -1
	end if
	
	do i=1,len_trim(fstr1)
		if ( fstr1(i:i) /= fstra(i:i) ) then
			write(error_unit,*)'ioda_c_string_to_f_dup failed?'
			write(error_unit,*)' orig ',trim(fstr1)
			write(error_unit,*)' copy ',trim(fstra)
			write(error_unit,*)' orig ',fstr1(i:i)
			write(error_unit,*)' copy ',fstra(i:i)
			stop -1
		end if	
	end do

	deallocate(fstra)
	call ioda_c_free(cstr2)
	call ioda_c_free(cstr1)

        call exit(0)
	
end program ioda_fortran_string_test
