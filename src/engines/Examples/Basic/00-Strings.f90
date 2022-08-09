!
! (C) Copyright 2021-2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Test the ioda Fortran string interface.

program ioda_fortran_string_test
    use, intrinsic :: iso_c_binding
    use :: ioda_f
    use :: ioda_string_f
    implicit none
    integer(c_size_t) :: str_retval
    character(kind=c_char,len=40) :: str_buf

    ! Create a new string
    type(ioda_string) :: str, str_copy
    str = ioda_string()

    ! Set a string and check that the length is correct.
    call str%set("This is a test.")

    if (str%size() /= 15) then
        stop 'Wrong size after setting a string.'
    end if

    ! Read a string and check that it has the correct value.
    str_retval = str%get(str_buf)
    if (str_retval /= 15) then
        stop 'String read had wrong number of characters.'
    end if
    write(*,*) str_buf

    ! Test to copy a string.
    str_copy = str
    str_retval = str_copy%get(str_buf)
    if (str_retval /= 15) then
        stop 'String copy read had wrong number of characters.'
    end if

    ! Test to clear a string.
    call str%clear()
    if (str%size() /= 0) then
        stop 'Wrong size after clearing a string.'
    end if

    ! Check that str_copy is unaffected by clearing str.
    str_retval = str_copy%get(str_buf)
    if (str_retval /= 15) then
        stop 'String copy read had wrong number of characters after str clear.'
    end if

end program ioda_fortran_string_test
