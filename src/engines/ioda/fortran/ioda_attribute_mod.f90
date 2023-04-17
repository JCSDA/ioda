!
! (C) Copyright 2023 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module ioda_attribute_mod
     use,intrinsic :: iso_c_binding
     use,intrinsic :: iso_fortran_env
     use :: ioda_dimensions_mod
     use :: ioda_vecstring_mod
      
     type :: ioda_attribute
         type(c_ptr) :: data_ptr = c_null_ptr
     contains
         final ioda_attribute_dtor
         procedure :: get_dimensions => ioda_attribute_get_dimensions

         procedure,private,pass(this) :: ioda_attribute_copy
         generic,public :: assignment(=) => ioda_attribute_copy

         procedure :: read_float => ioda_attribute_read_float
         procedure :: read_double => ioda_attribute_read_double
         procedure :: read_char => ioda_attribute_read_char
         procedure :: read_int16 => ioda_attribute_read_int16
         procedure :: read_int32 => ioda_attribute_read_int32
         procedure :: read_int64 => ioda_attribute_read_int64
         procedure :: read_str => ioda_attribute_read_str
       
         generic :: read=> &        
     		& read_float,read_double,read_char,read_int16, &
     		& read_int32,read_int64 

         procedure :: write_float => ioda_attribute_write_float
         procedure :: write_double => ioda_attribute_write_double
         procedure :: write_char => ioda_attribute_write_char
         procedure :: write_int16 => ioda_attribute_write_int16
         procedure :: write_int32 => ioda_attribute_write_int32
         procedure :: write_int64 => ioda_attribute_write_int64
         procedure :: write_str => ioda_attribute_write_str
       
         generic :: write=> &        
     		& write_float,write_double,write_char,write_int16, &
     		& write_int32,write_int64 
     		 
         procedure :: is_a_float => ioda_attribute_is_a_float
         procedure :: is_a_double => ioda_attribute_is_a_double
         procedure :: is_a_char => ioda_attribute_is_a_char
         procedure :: is_a_int16 => ioda_attribute_is_a_int16
         procedure :: is_a_int32 => ioda_attribute_is_a_int32
         procedure :: is_a_int64 => ioda_attribute_is_a_int64
         procedure :: is_a_str => ioda_attribute_is_a_str
         procedure :: is_a_uint16 => ioda_attribute_is_a_uint16
         procedure :: is_a_uint32 => ioda_attribute_is_a_uint32
         procedure :: is_a_uint64 => ioda_attribute_is_a_uint64

    end type 

interface

    function ioda_attribute_c_alloc() result(p) bind(C,name="ioda_attribute_c_alloc")
         import c_ptr
         type(c_ptr) :: p 
    end function

    subroutine ioda_attribute_c_dtor(p) bind(C,name="ioda_attribute_c_dtor")
         import c_ptr
         type(c_ptr) :: p
    end subroutine

    subroutine ioda_attribute_c_clone(this,rhs) bind(C,name="ioda_attribute_c_clone")
         import c_ptr
         type(c_ptr) :: this
         type(c_ptr),value :: rhs
    end subroutine
  
    function ioda_attribute_c_get_dimensions(p) result(dims) bind(C,name="ioda_attribute_c_get_dimensions")
         import c_ptr
         type(c_ptr),value :: p
         type(c_ptr) :: dims
    end function
    
    function ioda_attribute_c_write_float(p,n,data_) result(r) bind(C,name="ioda_attribute_c_write_float")
         import c_ptr,c_int64_t,c_bool,c_float
         type(c_ptr),value :: p
         real(c_float),dimension(:),intent(in) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
    
    function ioda_attribute_c_write_double(p,n,data_) result(r) bind(C,name="ioda_attribute_c_write_double")
         import c_ptr,c_int64_t,c_bool,c_double
         type(c_ptr),value :: p
         real(c_double),dimension(:),intent(in) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function

    function ioda_attribute_c_write_char(p,n,data_) result(r) bind(C,name="ioda_attribute_c_write_char")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
    
    function ioda_attribute_c_write_int16(p,n,data_) result(r) bind(C,name="ioda_attribute_c_write_int16")
         import c_ptr,c_int64_t,c_bool,c_int16_t
         type(c_ptr),value :: p
         integer(c_int16_t),dimension(:),intent(in) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function

    function ioda_attribute_c_write_int32(p,n,data_) result(r) bind(C,name="ioda_attribute_c_write_int32")
         import c_ptr,c_int64_t,c_bool,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t),dimension(:),intent(in) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
    
    function ioda_attribute_c_write_int64(p,n,data_) result(r) bind(C,name="ioda_attribute_c_write_int64")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         integer(c_int64_t),dimension(:),intent(in) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
        
    function ioda_attribute_c_write_str(p,data_) result(r) bind(C,name="ioda_attribute_c_write_str")
         import c_ptr,c_int64_t,c_bool,c_double
         type(c_ptr),value :: p
         type(c_ptr) :: data_
         logical(c_bool) :: r         
    end function

    function ioda_attribute_c_read_float(p,n,data_) result(r) bind(C,name="ioda_attribute_c_read_float")
         import c_ptr,c_int64_t,c_bool,c_float
         type(c_ptr),value :: p
         real(c_float),dimension(:),intent(out) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
    
    function ioda_attribute_c_read_double(p,n,data_) result(r) bind(C,name="ioda_attribute_c_read_double")
         import c_ptr,c_int64_t,c_bool,c_double
         type(c_ptr),value :: p
         real(c_double),dimension(:),intent(out) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function

    function ioda_attribute_c_read_char(p,n,data_) result(r) bind(C,name="ioda_attribute_c_read_char")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
    
    function ioda_attribute_c_read_int16(p,n,data_) result(r) bind(C,name="ioda_attribute_c_read_int16")
         import c_ptr,c_int64_t,c_bool,c_int16_t
         type(c_ptr),value :: p
         integer(c_int16_t),dimension(:),intent(out) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function

    function ioda_attribute_c_read_int32(p,n,data_) result(r) bind(C,name="ioda_attribute_c_read_int32")
         import c_ptr,c_int64_t,c_bool,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t),dimension(:),intent(out) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
    
    function ioda_attribute_c_read_int64(p,n,data_) result(r) bind(C,name="ioda_attribute_c_read_int64")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         integer(c_int64_t),dimension(:),intent(out) :: data_
         integer(c_int64_t),value :: n
         logical(c_bool) :: r         
    end function
        
    function ioda_attribute_c_read_str(p,data_) result(r) bind(C,name="ioda_attribute_c_read_str")
         import c_ptr,c_int64_t,c_bool,c_double
         type(c_ptr),value :: p
         type(c_ptr) :: data_
         logical(c_bool) :: r         
    end function
    
    function ioda_attribute_c_is_a_float(p) result(r) bind(C,name="ioda_attribute_c_is_a_float")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_double(p) result(r) bind(C,name="ioda_attribute_c_is_a_double")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_ldouble(p) result(r) bind(C,name="ioda_attribute_c_is_a_ldouble")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_char(p) result(r) bind(C,name="ioda_attribute_c_is_a_char")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_int16(p) result(r) bind(C,name="ioda_attribute_c_is_a_int16")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_int32(p) result(r) bind(C,name="ioda_attribute_c_is_a_int32")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_int64(p) result(r) bind(C,name="ioda_attribute_c_is_a_int64")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_str(p) result(r) bind(C,name="ioda_attribute_c_is_a_str")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_uint16(p) result(r) bind(C,name="ioda_attribute_c_is_a_uint16")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_uint32(p) result(r) bind(C,name="ioda_attribute_c_is_a_uint32")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

    function ioda_attribute_c_is_a_uint64(p) result(r) bind(C,name="ioda_attribute_c_is_a_uint64")     
         import c_ptr,c_int32_t
         type(c_ptr),value :: p
         integer(c_int32_t) :: r
    end function

end interface 
contains
    subroutine ioda_attribute_init(this) 
    	implicit none
    	type(ioda_attribute) :: this
!	this%data_ptr = ioda_attribute_c_alloc()
    end subroutine
    
    subroutine ioda_attribute_dtor(this)
        implicit none
        type(ioda_attribute) :: this
!        call ioda_attribute_c_dtor(this%data_ptr) 
    end subroutine
    
    subroutine ioda_attribute_copy(this,rhs)
         implicit none
         class(ioda_attribute),intent(in) :: rhs
         class(ioda_attribute),intent(out) :: this
	 call ioda_attribute_c_clone(this%data_ptr,rhs%data_ptr)
    end subroutine

    function ioda_attribute_get_dimensions(this) result(dims)
        implicit none
        class(ioda_attribute) :: this
        type(ioda_dimensions) :: dims
	dims%data_ptr = ioda_attribute_c_get_dimensions(this%data_ptr)
    end function
    
    logical function ioda_attribute_write_float(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        real(real32),dimension(:),intent(in) :: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_write_float(this%data_ptr,n,data_) 
    end function      
    
    logical function ioda_attribute_write_double(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        real(real64),dimension(:),intent(in) :: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_write_double(this%data_ptr,n,data_) 
    end function      
    
    logical function ioda_attribute_write_char(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        character(len=*),intent(in) :: data_ 
        integer(int64),intent(in) :: n
        type(c_ptr) :: cdata_ptr
        
        cdata_ptr = ioda_f_string_to_c_dup(data_)
        r = ioda_attribute_c_write_char(this%data_ptr,n,cdata_ptr) 
        call ioda_c_free(cdata_ptr) 
    end function      
    
    logical function ioda_attribute_write_int16(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        integer(int16),dimension(:),intent(in) :: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_write_int16(this%data_ptr,n,data_) 
    end function      
    
    logical function ioda_attribute_write_int32(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        integer(int32),dimension(:),intent(in):: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_write_int32(this%data_ptr,n,data_) 
    end function      

    logical function ioda_attribute_write_int64(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        integer(int64),dimension(:),intent(in):: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_write_int64(this%data_ptr,n,data_) 
    end function      

    logical function ioda_attribute_write_str(this,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        class(ioda_string),intent(in):: data_ 
        character(len=245) :: strp

        call data_%get(strp)
	write(error_unit,*)' write str ',strp
        r = ioda_attribute_c_write_str(this%data_ptr,data_%data_ptr) 
    end function      

    logical function ioda_attribute_read_float(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        real(real32),dimension(:),intent(out) :: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_read_float(this%data_ptr,n,data_) 
    end function      
    
    logical function ioda_attribute_read_double(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        real(real64),dimension(:),intent(out) :: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_read_double(this%data_ptr,n,data_) 
    end function      
    
    logical function ioda_attribute_read_char(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        character(len=*),intent(out) :: data_ 
        integer(int64),intent(in) :: n
        type(c_ptr) :: cdata_ptr
        
        cdata_ptr = ioda_c_string_alloc(n)
        r = ioda_attribute_c_read_char(this%data_ptr,n,cdata_ptr) 
        call ioda_c_string_to_f_copy(cdata_ptr,data_)
        call ioda_c_free(cdata_ptr) 
    end function      
    
    logical function ioda_attribute_read_int16(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        integer(int16),dimension(:),intent(out) :: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_read_int16(this%data_ptr,n,data_) 
    end function      
    
    logical function ioda_attribute_read_int32(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        integer(int32),dimension(:),intent(out) :: data_ 
        integer(int64),intent(in) :: n
        r = ioda_attribute_c_read_int32(this%data_ptr,n,data_) 
    end function      

    logical function ioda_attribute_read_int64(this,n,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        integer(int64),dimension(:),intent(out) :: data_ 
        integer(int64),intent(in) :: n
        
        r = ioda_attribute_c_read_int64(this%data_ptr,n,data_) 
    end function      

    logical function ioda_attribute_read_str(this,data_) result(r)
        implicit none
        class(ioda_attribute) :: this
        class(ioda_string),intent(out) :: data_ 	
        r = ioda_attribute_c_read_str(this%data_ptr,data_%data_ptr) 
    end function      

    integer(int32) function ioda_attribute_is_a_float(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_float(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_double(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_double(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_ldouble(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_ldouble(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_char(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_char(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_int16(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_int16(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_int32(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_int32(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_int64(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_int64(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_str(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_str(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_uint16(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_uint16(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_uint32(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_uint32(this%data_ptr)
    end function

    integer(int32) function ioda_attribute_is_a_uint64(this) result(r)
        implicit none
        class(ioda_attribute) :: this
        r = ioda_attribute_c_is_a_uint64(this%data_ptr)
    end function

end module
