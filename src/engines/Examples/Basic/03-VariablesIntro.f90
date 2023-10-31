!/*
! * (C) Copyright 2020-2021 UCAR
! *
! * This software is licensed under the terms of the Apache Licence Version 2.0
! * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! */
program variables_example
	use,intrinsic :: iso_fortran_env
	
	use :: ioda_variable_mod
	use :: ioda_has_variables_mod
	use :: cxx_vector_string_mod
        use :: ioda_group_mod
        use :: ioda_engines_mod
        use :: ioda_variable_creation_parameters_mod
        use :: ioda_dimensions_mod
                
        implicit none        
	type(ioda_group) :: g
	type(ioda_has_variables) :: gvars
	type(ioda_variable) :: ivar,dvar,zvar,ivar2
	type(ioda_variable_creation_parameters) :: p1
	type(cxx_vector_string) :: vlist
        type(ioda_dimensions) :: dims 
	logical :: r
	integer(int64) :: ns
	integer(int64) :: nd
	integer(int64),dimension(4) :: cdims
        integer(int64),dimension(4) :: mdims
	integer(int32),dimension(6) :: data_i
	real(real64),dimension(6) :: data_r
        character(len=256) :: name
        type(cxx_vector_string) :: vls
        integer(int64) :: i
            
!        call ioda_variable_init(ivar)
!        call ioda_variable_init(ivar2)
!        call ioda_variable_init(dvar)
!        call ioda_variable_init(zvar)
!        call ioda_has_variables_init(gvars)

        cdims(1) = 0
        cdims(2) = 0
        cdims(3) = 0
        cdims(4) = 0
                  	
        g = ioda_engines_construct_from_command_line('Example-03-f.hdf5')
        
        gvars = g%has_variables()
        
        write(error_unit,*)' have has_variables' 
        ns = 5
        nd = 2
        cdims(1) = 2
        cdims(2) = 3
	name = 'var-1'
        ivar = gvars%create_int32(name,nd,cdims)
        
        r = gvars%exists(ns,name)	
      	
      	if ( r .eqv. .false.) then
		write(error_unit,*)'fortran ioda_create_int var-1 variable does not exist'
                call cxx_vector_string_init(vls)
                vls = gvars%list() 		
		ns = vls%size()
		write(error_unit,*)' has_Var size = ',ns
		do i=1,ns
		   call vls%get(i,name)
		   write(error_unit,*) i, ' ' , name
		end do
		stop -1
	end if

        write(error_unit,*)' create_int32 worked'  
              
	nd = 6  
	data_i(1) = 1
	data_i(2) = 2
	data_i(3) = 3
	data_i(4) = 4
	data_i(5) = 5
	data_i(6) = 6
        r = ivar%write(nd,data_i)

        if (r .eqv. .false.) then
        	write(error_unit,*)'ioda_variable write int failed'
        	stop -1
        end if

	ns = 5
	nd = 3
	cdims(1) = 1
	cdims(2) = 2
	cdims(3) = 3
        dvar = gvars%create_double('var-2',nd,cdims)

	if ( gvars%exists(ns,'var-2') .eqv. .false.) then
		write(error_unit,*)'create_double var-2 variable does not exist'
		stop -1
	end if
        
        data_r(1) = 1.1
        data_r(2) = 2.2
        data_r(3) = 3.1416
        data_r(4) = 4.4
        data_r(5) = 5.5
        data_r(5) = 6.6
        ns = 6
        r = dvar%write(ns,data_r)       

        if ( r .eqv. .false.) then
        	write(error_unit,*)'ioda_variable write double failed'
                stop -1
        end if
                
        call p1%init()
        cdims(1) = 200
        cdims(2) = 3
        nd = 2
        call p1%chunking(.true.,nd,cdims)
        
        call p1%set_fill_value(-999)
        
        call p1%compress_with_gzip(4)
          
        ns = 5
        nd = 2
        cdims(1) = 200
        cdims(2) = 3
        mdims(1) = 2000
        mdims(2) = 3
        ivar2 = gvars%create2_int32('var-3',nd,cdims,mdims,p1)

	if ( gvars%exists(ns,'var-3') .eqv. .false.) then
		write(error_unit,*)'create2_int32 var-3 variable does not exist'
		stop -1
	end if


        call cxx_vector_string_init(vlist)
        vlist = gvars%list()
        
        if ( vlist%size() .ne. 3) then
        	write(error_unit,*)'ioda_has_variables list failed or creation failed'
                stop -1 
        end if         


	ns = 5
	zvar = gvars%open(ns,'var-2')
	
	dims = zvar%get_dimensions()
	
	if (dims%get_dimensionality() .ne. 3) then
		write(error_unit,*)'dimensions of variable is wrong'
	        stop -1
	end if
	
end program        


