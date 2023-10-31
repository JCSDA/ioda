!/*
! * (C) Copyright 2023 UCAR
! *
! * This software is licensed under the terms of the Apache Licence Version 2.0
! * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! */
module obsdatavector_mod
	use,intrinsic :: iso_c_binding
	use,intrinsic :: iso_fortran_env
	use :: cxx_string_mod
		
	type :: obsdatavector_int
		type(c_ptr) :: data_ptr = c_null_ptr
	contains
		procedure,pass(this) :: obsdatavector_int_init_from_ptr
		generic,public :: assignment(=) => obsdatavector_int_init_from_ptr
		procedure :: obsdatavector_int_get_row_i
		procedure :: obsdatavector_int_get_row_cxx_str
		procedure :: obsdatavector_int_get_row_str
		generic,public :: get_row => obsdatavector_int_get_row_i,&
					obsdatavector_int_get_row_str,&
					obsdatavector_int_get_row_cxx_str

		procedure :: nvars => obsdatavector_int_nvars
		procedure :: nlocs => obsdatavector_int_nlocs		
	end type
	interface
		function obsdatavector_int_c_get_row_i(p,i) result(r) &
			 bind(C,name="obsdatavector_int_c_get_row_i")
			import C_ptr,C_size_t
			type(c_ptr),value :: p
			integer(c_size_t),value :: i
			type(c_ptr) :: r
		end function
		function obsdatavector_int_c_get_row_cxx_str(p,s) result(r) &
			 bind(C,name="obsdatavector_int_c_get_row_i")
			import C_ptr
			type(c_ptr),value :: p
			type(c_ptr),value :: s
			type(c_ptr) :: r
		end function
		function obsdatavector_int_c_get_row_str(p,s) result(r) &
			 bind(C,name="obsdatavector_int_c_get_row_i")
			import C_ptr
			type(c_ptr),value :: p
			type(c_ptr),value :: s
			type(c_ptr) :: r
		end function
		function obsdatavector_int_c_get(p,i,j) result(r) bind(C,name="obsdatavector_int_c_get")
			import C_ptr,C_size_t,c_int
			type(c_ptr),value :: p
			integer(c_size_t),value :: i,j
			integer(c_int) :: r
		end function
		function obsdatavector_int_c_nlocs(p) result(n) bind(C,name="obsdatavector_int_c_nlocs")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t) :: n
		end function
		function obsdatavector_int_c_nvars(p) result(n) bind(C,name="obsdatavector_int_c_nlocs")
			import c_ptr,c_int64_t
			type(c_ptr),value :: p
			integer(c_int64_t) :: n
		end function
	end interface
contains
	subroutine obsdatavector_int_init_from_ptr(this,ptr)
		implicit none
		class(obsdatavector_int),intent(inout) :: this
		type(c_ptr),intent(in) :: ptr
		this%data_ptr = ptr
	end subroutine
	function obsdatavector_int_get_row_i(this,i) result(r)
		implicit none
		class(obsdatavector_int),intent(in) :: this
		integer(int64),intent(in) :: i 
		integer(int32),dimension(:),pointer :: r
		type(c_ptr) :: r_ptr
		integer(int64) :: j,nvars
		nvars = obsdatavector_int_c_nvars(this%data_ptr)
		j = i - 1
		r_ptr = obsdatavector_int_c_get_row_i(this%data_ptr,j)
		call c_f_pointer(r_ptr,r,[nvars])
	end function
	function obsdatavector_int_get_row_cxx_str(this,str) result(r)
		implicit none
		class(obsdatavector_int),intent(in) :: this
		class(cxx_string),intent(in) :: str
		integer(int32),dimension(:),pointer :: r
		type(c_ptr) :: r_ptr		
		integer(int64) :: nvars
		nvars = obsdatavector_int_c_nvars(this%data_ptr)
		r_ptr = obsdatavector_int_c_get_row_cxx_str(this%data_ptr,str%data_ptr)
		call c_f_pointer(r_ptr,r,[nvars])
	end function
	function obsdatavector_int_get_row_str(this,fstr) result(r)
		implicit none
		class(obsdatavector_int),intent(in) :: this
		character(len=*),intent(in) :: fstr
		integer(int32),dimension(:),pointer :: r
		type(c_ptr) :: r_ptr
		type(c_ptr) :: cstr_ptr		
		integer(int64) :: nvars
		nvars = obsdatavector_int_c_nvars(this%data_ptr)
		cstr_ptr = f_string_to_c_dup(fstr)
		r_ptr = obsdatavector_int_c_get_row_str(this%data_ptr,cstr_ptr)
		call c_free(cstr_ptr)
		call c_f_pointer(r_ptr,r,[nvars])
	end function
	function obsdatavector_int_get(this,i,j) result(r)
		implicit none
		class(obsdatavector_int),intent(in) :: this
		integer(int64),intent(in) :: i,j 
		integer(int32) :: r
		r = obsdatavector_int_c_get(this%data_ptr,i,j)
	end function
	function obsdatavector_int_nvars(this) result(n)
		implicit none
		class(obsdatavector_int),intent(in) :: this
		integer(int64) :: n
		n = obsdatavector_int_c_nvars(this%data_ptr)
	end function
	function obsdatavector_int_nlocs(this) result(n)
		implicit none
		class(obsdatavector_int),intent(in) :: this
		integer(int64) :: n
		n = obsdatavector_int_c_nlocs(this%data_ptr)
	end function
end module