!
! (C) Copyright 2023 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module ioda_has_attributes_mod
     use,intrinsic :: iso_c_binding
     use,intrinsic :: iso_fortran_env
     use :: ioda_dimensions_mod
     use :: cxx_vector_string_mod
     use :: ioda_attribute_mod
     use :: f_c_string_mod
     
     type :: ioda_has_attributes
         type(c_ptr) :: data_ptr = c_null_ptr
     contains
         final ioda_has_attributes_dtor

         procedure,private,pass(this) :: ioda_has_attributes_copy
         generic,public :: assignment(=) => ioda_has_attributes_copy
             
         procedure :: remove => ioda_has_attributes_remove
         procedure :: open => ioda_has_attributes_open
         procedure :: rename => ioda_has_attributes_rename
         procedure :: exists => ioda_has_attributes_exists
         procedure :: create_float => ioda_has_attributes_create_float
	 procedure :: list => ioda_has_attributes_list
         procedure :: create_double => ioda_has_attributes_create_double
         procedure :: create_char => ioda_has_attributes_create_char
         procedure :: create_int16 => ioda_has_attributes_create_int16
         procedure :: create_int32 => ioda_has_attributes_create_int32
         procedure :: create_int64 => ioda_has_attributes_create_int64
         procedure :: create_str => ioda_has_attributes_create_str
     end type

interface
    function ioda_has_attributes_c_alloc() result(p) bind(C,name="ioda_has_attributes_c_alloc")
         import c_ptr
         type(c_ptr) :: p 
    end function

    subroutine ioda_has_attributes_c_dtor(p) bind(C,name="ioda_has_attributes_c_dtor")
         import c_ptr
         type(c_ptr) :: p
    end subroutine
    
    subroutine ioda_has_attributes_c_clone(this,rhs) bind(C,name="ioda_has_attributes_c_clone")
	 import c_ptr
	 type(c_ptr),value :: rhs
	 type(c_ptr) :: this
    end subroutine	
    
    function ioda_has_attributes_c_list(p) result(vstr) bind(C,name="ioda_has_attributes_c_list")
    	import c_ptr
    	type(c_ptr),value :: p
    	type(c_ptr) :: vstr
    end function
    
    function ioda_has_attributes_c_rename(p,nold,cold,nnew,cnew) result(r) bind(C,name="ioda_has_attributes_c_rename")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: cold
         type(c_ptr),value :: cnew
         integer(c_int64_t),value :: nold,nnew
	 logical(c_bool) :: r
    end function
    
    function ioda_has_attributes_c_remove(p,n,str) result(r) bind(C,name="ioda_has_attributes_c_remove")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: str
         integer(c_int64_t),value :: n
         logical(c_bool) :: r
    end function

    function ioda_has_attributes_c_exists(p,n,str) result(r) bind(C,name="ioda_has_attributes_c_exists")
         import c_ptr,c_int64_t,c_int
         type(c_ptr),value :: p
         type(c_ptr),value :: str
         integer(c_int64_t),value :: n
         integer(c_int) :: r
    end function

    function ioda_has_attributes_c_open(p,n,str) result(att_ptr) bind(C,name="ioda_has_attributes_c_open")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: str
         type(c_ptr) :: att_ptr
         integer(c_int64_t),value :: n
    end function
   
    function ioda_has_attributes_c_create_float(p,n,s,nd,d,a) result(r) bind(C,name="ioda_has_attributes_c_create_float")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: s
         type(c_ptr) :: a
         integer(c_int64_t),dimension(*),intent(in) :: d
         integer(c_int64_t),value :: n,nd
         logical(c_bool) :: r 
    end function

    function ioda_has_attributes_c_create_double(p,n,s,nd,d,a) result(r) bind(C,name="ioda_has_attributes_c_create_double")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: s
         type(c_ptr) :: a
         integer(c_int64_t),dimension(*),intent(in) :: d
         integer(c_int64_t),value :: n,nd
         logical(c_bool) :: r 
    end function

    function ioda_has_attributes_c_create_char(p,n,s,nd,d,a) result(r) bind(C,name="ioda_has_attributes_c_create_char")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: s
         type(c_ptr) :: a
         integer(c_int64_t),dimension(*),intent(in) :: d
         integer(c_int64_t),value :: n,nd
         logical(c_bool) :: r 
    end function

    function ioda_has_attributes_c_create_int16(p,n,s,nd,d,a) result(r) bind(C,name="ioda_has_attributes_c_create_int16")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: s
         type(c_ptr) :: a
         integer(c_int64_t),dimension(*),intent(in) :: d
         integer(c_int64_t),value :: n,nd
         logical(c_bool) :: r 
    end function

    function ioda_has_attributes_c_create_int32(p,n,s,nd,d,a) result(r) bind(C,name="ioda_has_attributes_c_create_int32")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: s
         type(c_ptr) :: a
         integer(c_int64_t),dimension(*),intent(in) :: d
         integer(c_int64_t),value :: n,nd
         logical(c_bool) :: r 
    end function

    function ioda_has_attributes_c_create_int64(p,n,s,nd,d,a) result(r) bind(C,name="ioda_has_attributes_c_create_int64")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: s
         type(c_ptr) :: a
         integer(c_int64_t),dimension(*),intent(in) :: d
         integer(c_int64_t),value :: n,nd
         logical(c_bool) :: r 
    end function

    function ioda_has_attributes_c_create_str(p,n,s,nd,d,a) result(r) bind(C,name="ioda_has_attributes_c_create_str")
         import c_ptr,c_int64_t,c_bool
         type(c_ptr),value :: p
         type(c_ptr),value :: s
         type(c_ptr) :: a
         integer(c_int64_t),dimension(*),intent(in) :: d
         integer(c_int64_t),value :: n,nd
         logical(c_bool) :: r 
    end function

end interface 
contains

!    subroutine ioda_has_attributes_init(this) 
!     	  implicit none
!    	  type(ioda_has_attributes),intent(out) :: this
!    	  this%data_ptr = ioda_has_attributes_c_alloc()
!    end subroutine
    
    subroutine ioda_has_attributes_dtor(this)
          implicit none
          type(ioda_has_attributes) :: this
          call ioda_has_attributes_c_dtor(this%data_ptr)  
    end subroutine
    
    subroutine ioda_has_attributes_copy(this,rhs)
           implicit none
           class(ioda_has_attributes),intent(in) :: rhs
           class(ioda_has_attributes),intent(out) :: this

	   call ioda_has_attributes_c_clone(this%data_ptr,rhs%data_ptr)
    end subroutine

    function ioda_has_attributes_list(this) result(vstr) 
          implicit none
          class(ioda_has_attributes) :: this
          type(cxx_vector_string) :: vstr
          vstr%data_ptr =  ioda_has_attributes_c_list(this%data_ptr)
    end function

    integer(int32) function ioda_has_attributes_exists(this,n,fstr) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          integer(int64) :: n
          character(len=*),intent(in) :: fstr
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_exists(this%data_ptr,n,cstr)
          call c_free(cstr)
    end function

    function ioda_has_attributes_open(this,n,fstr) result(att)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute) :: att
          integer(int64) :: n
          character(len=*),intent(in) :: fstr
          logical :: r
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          att%data_ptr = ioda_has_attributes_c_open(this%data_ptr,n,cstr)
          call c_free(cstr)
    end function
    
    function ioda_has_attributes_remove(this,n,fstr) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          integer(int64) :: n
          character(len=*),intent(in) :: fstr
          logical :: r
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_remove(this%data_ptr,n,cstr)
          call c_free(cstr)
    end function

    logical function ioda_has_attributes_rename(this,nold,fstr_old,nnew,fstr_new) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          integer(int64),intent(in) :: nold,nnew
          character(len=*),intent(in) :: fstr_old
          character(len=*),intent(in) :: fstr_new
          type(c_ptr) :: cstr_new,cstr_old


          cstr_new = f_string_to_c_dup(fstr_new)
          cstr_old = f_string_to_c_dup(fstr_old)
          r = ioda_has_attributes_c_rename(this%data_ptr,nold,cstr_old,nnew,cstr_new)
          call c_free(cstr_old)
          call c_free(cstr_new)
    end function
    
    logical function ioda_has_attributes_create_float(this,n,fstr,nd,dims,att) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute),intent(out) :: att
          integer(int64),intent(in) :: nd,n
          character(len=*),intent(in) :: fstr
          integer(int64),dimension(:),intent(in) :: dims
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_create_float(this%data_ptr,n,cstr,nd,dims,att%data_ptr)
	  call c_free(cstr) 
     end function

    logical function ioda_has_attributes_create_double(this,n,fstr,nd,dims,att) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute),intent(out) :: att
          integer(int64),intent(in) :: nd,n
          character(len=*),intent(in) :: fstr
          integer(int64),dimension(:),intent(in) :: dims
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_create_double(this%data_ptr,n,cstr,nd,dims,att%data_ptr)
	  call c_free(cstr) 
     end function

    logical function ioda_has_attributes_create_char(this,n,fstr,nd,dims,att) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute),intent(out) :: att
          integer(int64),intent(in) :: nd,n
          character(len=*),intent(in) :: fstr
          integer(int64),dimension(:),intent(in) :: dims
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_create_char(this%data_ptr,n,cstr,nd,dims,att%data_ptr)
	  call c_free(cstr) 
     end function

    logical function ioda_has_attributes_create_int16(this,n,fstr,nd,dims,att) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute),intent(out) :: att
          integer(int64),intent(in) :: nd,n
          character(len=*),intent(in) :: fstr
          integer(int64),dimension(:),intent(in) :: dims
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_create_int16(this%data_ptr,n,cstr,nd,dims,att%data_ptr)
	  call c_free(cstr) 
     end function

    logical function ioda_has_attributes_create_int32(this,n,fstr,nd,dims,att) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute),intent(out) :: att
          integer(int64),intent(in) :: nd,n
          character(len=*),intent(in) :: fstr
          integer(int64),dimension(:),intent(in) :: dims
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_create_int32(this%data_ptr,n,cstr,nd,dims,att%data_ptr)
	  call c_free(cstr) 
     end function

    logical function ioda_has_attributes_create_int64(this,n,fstr,nd,dims,att) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute),intent(out) :: att
          integer(int64),intent(in) :: nd,n
          character(len=*),intent(in) :: fstr
          integer(int64),dimension(:),intent(in) :: dims
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_create_int64(this%data_ptr,n,cstr,nd,dims,att%data_ptr)
	  call c_free(cstr) 
     end function

    logical function ioda_has_attributes_create_str(this,n,fstr,nd,dims,att) result(r)
          implicit none
          class(ioda_has_attributes) :: this
          type(ioda_attribute),intent(out) :: att
          integer(int64),intent(in) :: nd,n
          character(len=*),intent(in) :: fstr
          integer(int64),dimension(:),intent(in) :: dims
          type(c_ptr) :: cstr

          cstr = f_string_to_c_dup(fstr)
          r = ioda_has_attributes_c_create_str(this%data_ptr,n,cstr,nd,dims,att%data_ptr)
	  call c_free(cstr) 
     end function

end module
