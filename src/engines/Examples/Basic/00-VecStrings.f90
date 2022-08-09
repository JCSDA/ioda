!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Test the ioda Fortran vector<string> interface

program ioda_fortran_vecstring_test
    use, intrinsic :: iso_c_binding
    use :: ioda_f
    use :: ioda_vecstring_f
    implicit none
    integer(c_size_t) :: str_retval
    character(kind=c_char,len=40) :: str_buf

    ! Create a new vecstring
    type(ioda_vecstring) :: v, v2
    v = ioda_vecstring()

    ! Resize the vector
    call v%resize(2)

    ! Check size of vector
    if (v%size() /= 2) then
        stop 'Wrong size after calling resize.'
    end if

    ! Set the strings in the vector
    str_retval = v%setFromCharArray(0, "This is a test.")
    str_retval = v%setFromCharArray(1, "This is another test.")

    ! Check the size of a string in the vector
    if (v%elementSize(0) /= 15) then
        stop 'Wrong size after setting a string.'
    end if

    ! Read strings from the vector and check that they are correct.
    str_retval = v%getAsCharArray(0, str_buf)
    if (trim(str_buf) /= "This is a test.") then
        stop 'String read failed 0.'
    end if
    str_retval = v%getAsCharArray(1, str_buf)
    if (trim(str_buf) /= "This is another test.") then
        write (*,*) str_buf
        stop 'String read failed 1.'
    end if

    ! Copy a VecString
    v2 = v

    ! Verify that v2 has the same number of elements as v.
    if (v%size() /= v2%size()) then
        stop 'Both v and v2 should have the same number of elements.'
    end if

    ! Clear v and verify that its size is zero.
    call v%clear()
    if (v%size() /= 0) then
        stop 'v should have zero elements.'
    end if

    ! Double check that v2 still has two elements.
    if (v2%size() /= 2) then
        stop 'v2 should have two elements.'
    end if

    ! Done!
end program ioda_fortran_vecstring_test
