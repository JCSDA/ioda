!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Attribute manipulation using the C interface.
!>  This example parallels the C++ example. See 02-Attributes.cpp for comments and walkthrough.

program ioda_fortran_02_attributes
    use, intrinsic :: iso_c_binding
    use :: ioda_f
    use :: ioda_vecstring_f
    use :: ioda_group_f
    use :: ioda_engines_f
    use :: ioda_attribute_f
    use :: ioda_has_attributes_f
    use :: ioda_dimensions_f
    implicit none

    type(ioda_engines) :: engines
    type(ioda_group) :: grpFromFile
    type(ioda_attribute) :: intatt1, intatt2, floatatt1, doubleatt1, stratt1, check_intatt2
    type(ioda_vecstring) :: strs, check_strs, att_list
    type(ioda_dimensions) :: dims
    logical(c_bool) :: res     ! Return value. Mostly unchecked in this example.
    integer(c_size_t) :: res1  ! Return value. Mostly unchecked in this example.

    ! Create a file
    !grpFromFile = engines%obsstore%createRootGroup() 
    grpFromFile = engines%hh%createFile("Example-02-F.hdf5", ioda_Engines_BackendCreateModes_Truncate_If_Exists)

    ! Create attributes. These are a bit less flexible than in C++.
    intatt1 = grpFromFile%atts%create_int("int-att-1", int((/ 1 /), c_size_t))
    if (intatt1%write( (/ 5 /) ) .eqv. .false.) then
      stop 'Attribute write failed'
    end if

    intatt2 = grpFromFile%atts%create_int("int-att-2", int( (/ 2 /), c_size_t ))
    res = intatt2%write( (/ 1, 2 /) )

    floatatt1 = grpFromFile%atts%create_float("float-att-1", int( (/ 2 /), c_size_t))
    res = floatatt1%write( (/ 3.1159, 2.78 /) )

    doubleatt1 = grpFromFile%atts%create_double("double-att-1", int( (/ 4 /), c_size_t))
    res = doubleatt1%write( (/ 1.1, 2.2, 3.3, 4.4 /) )

    strs = ioda_vecstring()
    call strs%resize(1)
    res1 = strs%setFromCharArray(0, "This is a test.")
    stratt1 = grpFromFile%atts%create_str("str-1", int((/ 1 /), c_size_t))
    res = stratt1%write(strs)

    ! Check attribute type
    if (stratt1%ioda_attribute_isA_str() .ne. 1) then
        stop 'Attribute type check failed'
    end if

    ! List attributes
    att_list = grpFromFile%atts%list()   ! TODO: fix return call instead of parameter
    if (att_list%size() /= 5) then
        stop 'Wrong number of attributes'
    end if

    ! Open an attribute
    check_intatt2 = grpFromFile%atts%open('int-att-2')

    ! Get dimensionality and dimensions
    dims = check_intatt2%getDimensions()
    if (dims%getDimensionality() .ne. 1) then
        stop 'Attribute dimensionality check failed'
    end if
    if (dims%getNumElements() .ne. 2) then
        stop 'Attribute has wrong number of elements'
    end if
    if (dims%getDimCur(0) .ne. 2) then
        stop 'Attribute has wrong size along dimension 0'
    end if

    ! Read a numeric attribute

    ! Read a string attribute
    !check_strs = ioda_vecstring()
    !res = stratt1%read(check_strs)
    !if (strs%size() /= check_strs%size()) then
    !    stop 'Both strs and check_strs should have the same number of elements.'
    !end if


    ! Done!
end program ioda_fortran_02_attributes
