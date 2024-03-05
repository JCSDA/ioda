!
! (C) Copyright 2023 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
module ioda_variable_mod
    use,intrinsic :: iso_c_binding
    use,intrinsic :: iso_fortran_env
    use :: ioda_dimensions_mod
    use :: ioda_has_attributes_mod
    use :: cxx_vector_string_mod
    use :: f_c_string_mod
    
    type ioda_variable
        type(c_ptr) :: data_ptr = c_null_ptr
    contains   
        final ioda_variable_dtor

         procedure,private,pass(this) ::ioda_variable_copy
         generic,public :: assignment(=) => ioda_variable_copy

        procedure ::  has_attributes => ioda_variable_has_attributes
	procedure ::  get_dimensions => ioda_variable_get_dimensions
	procedure ::  resize => ioda_variable_resize
	procedure ::  attach_dim_scale => ioda_variable_attach_dim_scale
	procedure ::  detach_dim_scale => ioda_variable_detach_dim_scale
	procedure ::  set_dim_Scale => ioda_variable_set_dim_scale
	procedure ::  is_dim_scale => ioda_variable_is_dim_scale
        procedure ::  get_dim_scale_name => ioda_variable_get_dim_scale_name
        procedure ::  is_dim_scale_attached => ioda_variable_is_dim_scale_attached
! is_a        
        procedure ::  is_a_float => ioda_variable_is_a_float	
        procedure ::  is_a_double => ioda_variable_is_a_double	
        procedure ::  is_a_ldouble => ioda_variable_is_a_ldouble	
        procedure ::  is_a_char => ioda_variable_is_a_char	
        procedure ::  is_a_int16 => ioda_variable_is_a_int16	
        procedure ::  is_a_int32 => ioda_variable_is_a_int32	
        procedure ::  is_a_int64 => ioda_variable_is_a_int64
        procedure ::  is_a_uint16 => ioda_variable_is_a_uint16	
        procedure ::  is_a_uint32 => ioda_variable_is_a_uint32	
        procedure ::  is_a_uint64 => ioda_variable_is_a_uint64
        procedure ::  is_a_str => ioda_variable_is_a_str	
! read
        procedure ::  write_float => ioda_variable_write_float	
        procedure ::  write_double => ioda_variable_write_double	
        procedure ::  write_char => ioda_variable_write_char	
        procedure ::  write_int16 => ioda_variable_write_int16	
        procedure ::  write_int32 => ioda_variable_write_int32	
        procedure ::  write_int64 => ioda_variable_write_int64
        procedure ::  write_str => ioda_variable_write_str	

        generic :: write => write_float,write_double, write_char,&
             & write_int16,write_int32,write_int64
! write
        procedure ::  read_float => ioda_variable_read_float	
        procedure ::  read_double => ioda_variable_read_double	
        procedure ::  read_char => ioda_variable_read_char	
        procedure ::  read_int16 => ioda_variable_read_int16	
        procedure ::  read_int32 => ioda_variable_read_int32	
        procedure ::  read_int64 => ioda_variable_read_int64
        procedure ::  read_str => ioda_variable_read_str	

        generic :: read => read_float,read_double, read_char,&
             & read_int16,read_int32,read_int64
    end type

interface
    function  ioda_variable_c_alloc() result(p) bind(C,name="ioda_variable_c_alloc")
      import C_ptr
      type(C_ptr) :: p  
    end function
    
    subroutine ioda_variable_c_dtor(p) bind(C,name="ioda_variable_c_dtor")
      import C_ptr
      type(C_ptr) :: p  
    end subroutine 
    
    subroutine ioda_variable_c_clone(this,rhs) bind(C,name="ioda_variable_c_clone") 
      import c_ptr
      type(c_ptr),value :: rhs 
      type(c_ptr) :: this
    end subroutine	
     
    function ioda_variable_c_has_attributes(p) result(has_att) bind(C,name="ioda_variable_c_has_attributes")
      import C_ptr
      type(C_ptr),value :: p
      type(C_ptr) :: has_att
    end function  

    function ioda_variable_c_get_dimensions(p) result(dims_ptr) bind(C,name="ioda_variable_c_get_dimensions")
      import C_ptr
      type(C_ptr),value :: p
      type(C_ptr) :: dims_ptr
    end function
    
    function ioda_variable_c_resize(p,n,dims) result(r) bind(C,name="ioda_variable_c_resize")
      import C_ptr,c_int64_t,c_bool
      type(C_ptr),value :: p
      integer(c_int64_t),dimension(*),intent(in) :: dims	
      integer(c_int64_t),value :: n
      logical(c_bool) :: r     
    end function

    function ioda_variable_c_attach_dim_scale(p,dim_n,var_ptr) result(r) &
    	& bind(C,name="ioda_variable_c_attach_dim_scale")
      import C_ptr,c_int32_t,c_bool
      type(C_ptr),value :: p
      type(C_ptr),value :: var_ptr
      integer(c_int32_t),value :: dim_n     
      logical(c_bool) :: r
    end function 

    function ioda_variable_c_detach_dim_scale(p,dim_n,var_ptr) result(r) &
    	& bind(C,name="ioda_variable_c_detach_dim_scale")
      import C_ptr,c_int32_t,c_bool
      type(C_ptr),value :: p
      type(C_ptr),value :: var_ptr
      integer(c_int32_t),value :: dim_n     
      logical(c_bool) :: r
    end function 

    function ioda_variable_c_set_dim_scale(p,n,var_ptr) result(r) &
        & bind(C,name="ioda_variable_c_set_dim_scale") 
      import c_ptr,c_bool,c_int64_t
      type(c_ptr),value :: p
      type(c_ptr),value :: var_ptr
      integer(c_int64_t) :: n
      logical(c_bool) ::  r
    end function

    function ioda_variable_c_is_dim_scale(p) result(r) &
    	& bind(C,name="ioda_variable_c_is_dim_scale")
      import C_ptr,c_bool
      type(C_ptr),value :: p
      logical(c_bool) :: r
    end function 

    function ioda_variable_c_set_is_dim_scale(p,sz,dims_name) result(r) &
    	& bind(C,name="ioda_variable_c_set_is_dimension_scale")
      import C_ptr,c_int64_t,c_bool
      type(C_ptr),value :: p
      type(C_ptr),value :: dims_name
      integer(c_int64_t),value :: sz     
      logical(c_bool) :: r
    end function 

    function ioda_variable_c_get_dim_scale_name(p,n,out_str) result(r) &
        & bind(C,name="ioda_variable_c_get_dimension_scale_name")
        import c_ptr,c_int64_t
        type(c_ptr),value :: p
        integer(c_int64_t),value :: n
        type(c_ptr):: out_str
        integer(c_int64_t) :: r
    end function


    function ioda_variable_c_is_dim_scale_attached(p,dim_n,var_ptr) result(r) &
    	& bind(C,name="ioda_variable_c_is_dimension_scale_attached")
      import C_ptr,c_int32_t
      type(C_ptr),value :: p
      type(C_ptr),value :: var_ptr
      integer(c_int32_t),value :: dim_n    
      integer(C_int32_t) :: r
    end function 
        
    function ioda_variable_c_is_a_float(p) result(r) bind(C,name="ioda_variable_c_is_a_float")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_double(p) result(r) bind(C,name="ioda_variable_c_is_a_double")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_ldouble(p) result(r) bind(C,name="ioda_variable_c_is_a_ldouble")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_char(p) result(r) bind(C,name="ioda_variable_c_is_a_char")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_int16(p) result(r) bind(C,name="ioda_variable_c_is_a_int16")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_int32(p) result(r) bind(C,name="ioda_variable_c_is_a_int32")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_int64(p) result(r) bind(C,name="ioda_variable_c_is_a_int64")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_uint16(p) result(r) bind(C,name="ioda_variable_c_is_a_uint16")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_uint32(p) result(r) bind(C,name="ioda_variable_c_is_a_uint32")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_uint64(p) result(r) bind(C,name="ioda_variable_c_is_a_uint64")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_is_a_str(p) result(r) bind(C,name="ioda_variable_c_is_a_str")
        import C_ptr,c_int32_t
        type(C_ptr),value :: p
        integer(c_int32_t) :: r
    end function          

    function ioda_variable_c_write_float(p,n,data_) result(r) bind(C,name="ioda_variable_c_write_float")
        import C_ptr,c_bool,c_int64_t,c_float
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	real(c_float),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_write_double(p,n,data_) result(r) bind(C,name="ioda_variable_c_write_double")
        import C_ptr,c_bool,c_int64_t,c_double
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	real(c_double),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_write_char(p,n,data_) result(r) bind(C,name="ioda_variable_c_write_char")
        import C_ptr,c_bool,c_int64_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	type(C_ptr),value :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_write_int16(p,n,data_) result(r) bind(C,name="ioda_variable_c_write_int16")
        import C_ptr,c_bool,c_int64_t,c_int16_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	integer(c_int16_t),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_write_int32(p,n,data_) result(r) bind(C,name="ioda_variable_c_write_int32")
        import C_ptr,c_bool,c_int64_t,c_int32_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	integer(c_int32_t),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_write_int64(p,n,data_) result(r) bind(C,name="ioda_variable_c_write_int64")
        import C_ptr,c_bool,c_int64_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	integer(c_int64_t),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_write_str(p,vstr) result(r) bind(C,name="ioda_variable_c_write_str")
        import C_ptr,c_bool
        type(C_ptr),value :: p
        type(C_ptr),value :: vstr
        logical(c_bool) :: r
    end function

    function ioda_variable_c_read_float(p,n,data_) result(r) bind(C,name="ioda_variable_c_read_float")
        import C_ptr,c_bool,c_int64_t,c_float
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	real(c_float),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_read_double(p,n,data_) result(r) bind(C,name="ioda_variable_c_read_double")
        import C_ptr,c_bool,c_int64_t,c_double
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	real(c_double),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_read_char(p,n,data_) result(r) bind(C,name="ioda_variable_c_read_char")
        import C_ptr,c_bool,c_int64_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	type(C_ptr) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_read_int16(p,n,data_) result(r) bind(C,name="ioda_variable_c_read_int16")
        import C_ptr,c_bool,c_int64_t,c_int16_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	integer(c_int16_t),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_read_int32(p,n,data_) result(r) bind(C,name="ioda_variable_c_read_int32")
        import C_ptr,c_bool,c_int64_t,c_int32_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	integer(c_int32_t),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_read_int64(p,n,data_) result(r) bind(C,name="ioda_variable_c_read_int64")
        import C_ptr,c_bool,c_int64_t
        type(C_ptr),value :: p
	integer(c_int64_t),value :: n
	integer(c_int64_t),dimension(*),intent(out) :: data_
        logical(C_bool) :: r
    end function	

    function ioda_variable_c_read_str(p,vstr) result(r) bind(C,name="ioda_variable_c_read_str")
        import C_ptr,c_bool
        type(C_ptr),value :: p
        type(C_ptr) :: vstr
        logical(C_bool) :: r
    end function


end interface

contains
 
    subroutine ioda_variable_init(this)
       type(ioda_variable) :: this
       this%data_ptr = c_null_ptr
    end subroutine

    subroutine ioda_variable_dtor(this)
       type(ioda_variable) :: this
!       call ioda_variable_c_dtor(this%data_ptr)    
    end subroutine

    subroutine ioda_variable_copy(this,rhs)
       implicit none
       class(ioda_variable),intent(in) :: rhs
       class(ioda_variable),intent(out) :: this
       call ioda_variable_c_clone(this%data_ptr,rhs%data_ptr)
    end subroutine

    function ioda_variable_has_attributes(this) result(atts)
      implicit none
      class(ioda_variable) :: this
      type(ioda_has_attributes) :: atts
      atts%data_ptr =  ioda_variable_c_has_attributes(this%data_ptr)      
    end function

    function ioda_variable_get_dimensions(this) result(dims)
      implicit none
      class(ioda_variable) :: this
      type(ioda_dimensions) :: dims
      dims%data_ptr =  ioda_variable_c_get_dimensions(this%data_ptr)      
    end function

    function ioda_variable_resize(this,n,dims) result(r)
       implicit none
       class(ioda_variable) :: this
       integer(int64),intent(in) :: n
       integer(int64),dimension(:),intent(in) :: dims
       logical :: r
       r =  ioda_variable_c_resize(this%data_ptr,n,dims)  
    end function

    logical function ioda_variable_attach_dim_scale(this,n,scale) result(r)
       implicit none
       class(ioda_variable) :: this
       integer(int32),intent(in) :: n
       class(ioda_variable),intent(in) :: scale
       r =  ioda_variable_c_attach_dim_scale(this%data_ptr,n,scale%data_ptr)    
    end function
     
    logical function ioda_variable_detach_dim_scale(this,n,scale) result(r)
       implicit none
       class(ioda_variable) :: this
       integer(int32),intent(in) :: n
       class(ioda_variable),intent(in) :: scale
       r =  ioda_variable_c_detach_dim_scale(this%data_ptr,n,scale%data_ptr)    
    end function

    logical function ioda_variable_set_dim_scale(this,n,var) result(r)
       implicit none
       class(ioda_variable) :: this
       class(ioda_variable),intent(in) :: var
       integer(int64) :: n
       r = ioda_variable_c_set_dim_scale(this%data_ptr,n,var%data_ptr) 
    end function
	
    logical function ioda_variable_is_dim_scale(this) result(r)
       implicit none
       class(ioda_variable) :: this
       r = ioda_variable_c_is_dim_scale(this%data_ptr)	
    end function 
    
    logical function ioda_variable_set_is_dim_scale(this,n,name) result(r)
       implicit none
       class(ioda_variable) :: this
       character(len=*),intent(in) :: name
       integer(int64),intent(in) :: n
       type(C_ptr) :: name_ptr

       name_ptr = f_string_to_c_dup(name)
       r = ioda_variable_c_set_is_dim_scale(this%data_ptr,n,name_ptr)
       call c_free(name_ptr)    
    end function
         
    integer(int64) function ioda_variable_get_dim_scale_name(this,sz,name) result(r)
       implicit none
       class(ioda_variable) :: this
       character(len=*),intent(out) :: name
       integer(int64),intent(in) :: sz
       type(C_ptr) :: name_ptr
        
       name_ptr = c_alloc((sz+1))
       r = ioda_variable_c_get_dim_scale_name(this%data_ptr,sz,name_ptr)
       call c_string_to_f_copy(name_ptr,name)
       call c_free(name_ptr)     
    end function     
         
    integer(int32) function ioda_variable_is_dim_scale_attached(this,dim_n,vscale) result(r)  
        class(ioda_variable) :: this
        class(ioda_variable),intent(in) :: vscale
        integer(int32),intent(in) :: dim_n;

        r = ioda_variable_c_is_dim_scale_attached(this%data_ptr,dim_n,vscale%data_ptr)
    end function
        
    integer(int32) function ioda_variable_is_a_float(this) result(r)
        class(ioda_variable) :: this
        r = ioda_variable_c_is_a_float(this%data_ptr)
    end function 

    function ioda_variable_is_a_double(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_double(this%data_ptr)
    end function 

    function ioda_variable_is_a_ldouble(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_ldouble(this%data_ptr)
    end function 

    function ioda_variable_is_a_char(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_char(this%data_ptr)
    end function 

    function ioda_variable_is_a_int16(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_int32(this%data_ptr)
    end function 
    
    function ioda_variable_is_a_int32(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_int32(this%data_ptr)
    end function 
    
    function ioda_variable_is_a_int64(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_int64(this%data_ptr)
    end function 

    function ioda_variable_is_a_str(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_str(this%data_ptr)
    end function 

    function ioda_variable_is_a_uint16(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_uint32(this%data_ptr)
    end function 
    
    function ioda_variable_is_a_uint32(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_uint32(this%data_ptr)
    end function 
    
    function ioda_variable_is_a_uint64(this) result(r)
        class(ioda_variable) :: this
        integer(int32) :: r
        r = ioda_variable_c_is_a_uint64(this%data_ptr)
    end function 

    function ioda_variable_write_float(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        real(real32),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_write_float(this%data_ptr,n,data_)
    end function
    
    function ioda_variable_write_double(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        real(real64),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_write_double(this%data_ptr,n,data_) 
    end function

    function ioda_variable_write_int16(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        integer(int16),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_write_int16(this%data_ptr,n,data_)
    end function

    function ioda_variable_write_int32(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        integer(int32),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_write_int32(this%data_ptr,n,data_)
    end function
    
    function ioda_variable_write_int64(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        integer(int64),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_write_int64(this%data_ptr,n,data_)
    end function

    function ioda_variable_write_char(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
	character(len=*),intent(in) :: data_
        logical :: r
        type(c_ptr) :: data_p
        
        data_p = f_string_to_c_dup(data_)
        r = ioda_variable_c_write_char(this%data_ptr,n,data_p)
        call c_free(data_p)
    end function
    
    function ioda_variable_write_str(this,vstr) result(r)
        class(ioda_variable) :: this
        class(cxx_vector_string),intent(in) :: vstr
        logical :: r
        r = ioda_variable_c_write_str(this%data_ptr,vstr%data_ptr) 
    end function
    
    function ioda_variable_read_float(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        real(real32),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_read_float(this%data_ptr,n,data_)
    end function
    
    function ioda_variable_read_double(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        real(real64),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_read_double(this%data_ptr,n,data_)
    end function
 
    function ioda_variable_read_int16(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        integer(int16),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_read_int16(this%data_ptr,n,data_)
    end function

    function ioda_variable_read_int32(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        integer(int32),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_read_int32(this%data_ptr,n,data_)
    end function
    
    function ioda_variable_read_int64(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
        integer(int64),dimension(:),intent(out) :: data_
        logical :: r
        r = ioda_variable_c_read_int64(this%data_ptr,n,data_)
    end function

    function ioda_variable_read_char(this,n,data_) result(r)
        class(ioda_variable) :: this
        integer(int64),intent(in) :: n
	character(len=*),intent(out) :: data_
        logical :: r
        type(c_ptr) :: data_p

        data_p = c_alloc((n+1))
        r = ioda_variable_c_read_char(this%data_ptr,n,data_p)
        call c_string_to_f_copy(data_p,data_) 
        call c_free(data_p)
    end function
    
    function ioda_variable_read_str(this,vstr) result(r)
        class(ioda_variable) :: this
        class(cxx_vector_string),intent(out) :: vstr
        logical :: r
        r = ioda_variable_c_read_str(this%data_ptr,vstr%data_ptr) 
    end function
    
end module
