!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Attribute manipulation using the C interface.
!>  This example parallels the C++ example. See 02-Attributes.cpp for comments and walkthrough.

program ioda_fortran_02_attributes
    use, intrinsic :: iso_fortran_env
    use :: ioda_f_c_string_mod
    use :: ioda_vecstring_mod
    use :: ioda_group_mod
    use :: ioda_engines_mod
    use :: ioda_attribute_mod
    use :: ioda_has_attributes_mod
    use :: ioda_dimensions_mod
    implicit none

    type(ioda_group) :: grpFromFile
    type(ioda_has_attributes) :: g_has_att
    type(ioda_attribute) :: intatt1, intatt2, floatatt1, doubleatt1, stratt1, check_intatt2
    type(ioda_vecstring) :: check_strs, att_list
    type(ioda_string) :: xstr
    type(ioda_dimensions) :: dims
    logical :: res1     ! Return value. Mostly unchecked in this example.
    integer(int64),dimension(6) :: cdims
    integer(int32),dimension(2) :: wint
    real(real32),dimension(4) :: fint
    integer(int64) :: ns,nd;
    character(len=256) :: test_str
    
    test_str  = 'this is a test'         
    ! Create a file
    !grpFromFile = engines%obsstore%createRootGroup()
    grpFromFile = ioda_engines_hh_create_file('Example-02-F.hdf5', 1)

!    call ioda_attribute_init(intatt1)
!    call ioda_attribute_init(intatt2)
!    call ioda_attribute_init(floatatt1)
!    call ioda_attribute_init(stratt1)
!    call ioda_attribute_init(doubleatt1)
!    call ioda_attribute_init(check_intatt2)
!    call ioda_has_attributes_init(g_has_att)

    g_has_att = grpFromFile%has_attributes() 
    ! Create attributes. These are a bit less flexible than in C++.
    cdims(1) = 1
    ns = 9
    nd = 1
    res1 = g_has_att%create_int32(ns,'int-att-1', nd, cdims, intatt1)
    if ( res1 .eqv. .false.) then
       write(error_unit,*)'first has_attributes create int failed'
       stop -1
    end if
    wint(1) = 5
    res1 = intatt1%write(nd,wint)
    if (res1 .eqv. .false.) then
      write(error_unit,*)'Attribute write failed'
      stop -1	
    end if

    cdims(1) = 2 
    res1 = g_has_att%create_int32(ns,'int-att-2', nd, cdims, intatt2)
    if ( res1 .eqv. .false.) then
    	write(error_unit,*)'has_attributes create_int32 failed'
        stop -1
    end if
    wint(1) = 1
    wint(2) = 2
    nd = 2
    res1 = intatt2%write(nd,wint )

    ns = 10
    cdims(1) = 2
    nd = 1
    res1 = g_has_att%create_float(ns,'float-att-1', nd, cdims, floatatt1)
    if ( res1 .eqv. .false.) then
    	write(error_unit,*)'has_attributes create_float failed'
        stop -1
    end if

    fint(1) = 2.35e0
    fint(2) = 3.1415927e0     
    nd = 2
    res1 = floatatt1%write(nd,fint )

    cdims(1) = 1 
    ns = 5
    nd = 1
    res1 = g_has_att%create_str(ns,'str-1', nd, cdims ,stratt1)
    if ( res1 .eqv. .false.) then
    	write(error_unit,*)'f has_attributes create_str failed'
        stop -1
    end if

    call ioda_string_init(xstr)
    call xstr%set(test_str)
    res1 = stratt1%write_str(xstr)
    if ( res1 .eqv. .false.) then
    	write(error_unit,*)'f attributes write_str failed'
        stop -1
    end if

    ! Check attribute type
    if (stratt1%is_a_str() .ne. 1) then
        write(error_unit,*)'Attribute type check failed'
        stop -1
    end if

    ! List attributes
    call ioda_vecstring_init(att_list);
    att_list = g_has_att%list()  
    if (att_list%size() /= 4) then
        write(error_unit,*)'Wrong number of attributes'
        stop -1
    end if

    ! Open an attribute

    ns = 9
    check_intatt2 = g_has_att%open(ns,'int-att-2')
    ! Get dimensionality and dimensions
    dims =  check_intatt2%get_dimensions()
    if (dims%get_dimensionality() .ne. 1) then
        write(error_unit,*)'Attribute dimensionality check failed'
        stop -1  
    end if
    if (dims%get_num_elements() .ne. 2) then
        write(error_unit,*)'Attribute has wrong number of elements'
        stop -1  
    end if
    if (dims%get_dims_cur_size() .ne. 1) then
        write(error_unit,*)'Attribute has wrong # of current dimensions'
        stop -1 
    end if

end program ioda_fortran_02_attributes
