!/*
! * (C) Copyright 2022-2023 UCAR
! *
! * This software is licensed under the terms of the Apache Licence Version 2.0
! * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! */
program vecstring_test
	use,intrinsic :: iso_fortran_env
	use,intrinsic :: iso_c_binding
	use :: f_c_string_mod
	use :: cxx_string_mod
	use :: cxx_vector_string_mod
	implicit none
	type(cxx_vector_string) :: v1
	type(cxx_vector_string) :: v2
	type(cxx_vector_string) :: v3
	type(cxx_vector_string) :: v4
	type(cxx_string) :: s1
	type(cxx_string) :: s2
	type(cxx_string) :: s3
	character(len=256) :: a1
	character(len=256) :: a2
	character(len=256) :: a3
	character(len=256) :: a4
	character(len=256) :: a5
	character(len=256),dimension(4) :: arr
	integer(int64) :: i
	integer(int64) :: n,j

	a1 = 'this is a test'
	a2 = 'this is a second test'
	a3 = 'another string to test'
		
	arr(1)(:) = a1
	arr(2)(:) = a2
	arr(3)(:) = a3
		
	if ( .not. v1%empty() ) then
		write(error_unit,*)' empty failed'
	end if	
	write(error_unit,*)'testing push back'
	write(error_unit,*)' '

	call v1%push_back(a1)
	call v1%push_back(a2)
	call s1%set(a3)
	call v1%push_back(s1)
	if ( v1%size() /= 3) then
		write(error_unit,*)' push back failed'
		j = v1%size()
		write(error_unit,*)j,' != 3'
		stop -1
	end if	
	do i=1,3
		call v1%get(i,a4)
		if ( len_trim(a4) /= len_trim(arr(i))) then
			write(error_unit,*)' push back/get failed for i = ',i
			write(error_unit,*)' c = ',trim(a4),' ',len_trim(a4)
			write(error_unit,*)' f = ',trim(arr(i)),' ',len_trim(arr(i))
			stop -1
		end if
		write(error_unit,*)trim(a4),' = ',trim(arr(i))
	end do
	do i=1,3
		call v1%get(i,s2)
		if ( s2%size() /= len_trim(arr(i))) then
			write(error_unit,*)' push_back/get_str failed'
			stop -1
		end if
		call s2%get(a4)
		write(error_unit,*)trim(a4),' = ',trim(arr(i))
	end do	
	
	write(error_unit,*)'testing copy'
	write(error_unit,*)' '
	v2 = v1
	if ( v2%size() /= v1%size()) then
		write(error_unit,*)'copy failed sizes not equal'
		stop -1
	end if
	do i=1,3
		call v1%get(i,a4)
		call v2%get(i,a5)
		if ( len_trim(a4) /= len_trim(a5)) then
			write(error_unit,*)' copy/get failed'
			stop -1
		end if
	end do

	write(error_unit,*)'testing set'
	write(error_unit,*)' '

	j = 3
	call v3%resize(j)
	if ( v3%size() /= j) then
		write(error_unit,*)' resize failed'
		stop -1
	end if

	do i=1,3
		call v3%set(i,arr(i))
	end do
	do i=1,3
		call v1%get(i,a3)
		if ( len_trim(a3) /= len_trim(arr(i))) then
			write(error_unit,*)' set/get failed'
			stop -1
		end if
	end do
	do i=1,3
		call v1%get(i,s2)
		if ( s2%size() /= len_trim(arr(i))) then
			write(error_unit,*)' set/get_str failed'
			stop -1
		end if
	end do	

	do i=1,3
		call s3%set(arr(i))	
		call v3%set(i,s3)
	end do
	do i=1,3
		call v3%get(i,a3)
		if ( len_trim(a3) /= len_trim(arr(i))) then
			write(error_unit,*)' set/get failed'
			stop -1
		end if
	end do
	do i=1,3
		call v3%get(i,s2)
		if ( s2%size() /= len_trim(arr(i))) then
			write(error_unit,*)' set/get_str failed'
			stop -1
		end if
	end do	

	if ( v1%empty() ) then
		write(error_unit,*)' empty failed'
	end if
	call v1%clear()
	j = 0
	if ( v1%size() /= j) then
		write(error_unit,*)' clear failed'
	end if	
	
end program