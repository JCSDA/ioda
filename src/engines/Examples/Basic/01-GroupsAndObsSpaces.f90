!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Group manipulation using the Fortran interface. This example parallels the C++ example.

program ioda_fortran_01_groupsandobsspaces
    use, intrinsic :: iso_c_binding
    use, intrinsic :: iso_fortran_env
    use :: ioda_vecstring_mod
    use :: ioda_group_mod
    use :: ioda_engines_mod
    implicit none

    type(ioda_group) :: groot, g1, g2, g3, g4, g5, g6, g7, g8, reopened_g3
    type(ioda_vecstring) :: listed_groups_g3,listed_groups_g4
    integer(int64) :: nsz
    integer(int64) :: msz
    character(len=*),parameter :: root_file = 'Example-01.hdf5'
    ! Create a file
    !grpFromFile = engines%obsstore%createRootGroup()
    integer :: create_mode
    
    create_mode = 1             

    call ioda_engines_construct_from_command_line(groot,root_file) 
    
    nsz = 2
    msz = 5
    call groot%create(nsz,'g1',g1)
    call groot%create(nsz,'g2',g2)
    call g1%create(nsz,'g3',g3)
    call g3%create(nsz,'g4',g4)
    call g4%create(nsz,'g5',g5)
    call g4%create(nsz,'g6',g6)
    call groot%create(msz,'g7/g8',g7)
 
    ! Check for existence of groups
    if (g1%exists(nsz,'g3') .eq. 0) then
        write(error_unit,*) '/g1/g3 does not exist'
        stop -1
    end if

    if (groot%exists(msz,'g7/g8') .eq. 0) then
        write(error_unit,*) '/groo/g7/g8 does not exist'
        stop -1
    end if

    call ioda_vecstring_init(listed_groups_g3)	
    ! Listing groups contained within a group. See VecString example for full usage.
    call g3%list(listed_groups_g3)
    if (listed_groups_g3%size() /= 1) then
        write(error_unit,*) 'Unexpected number of child groups for g3.'
        stop -1
    end if
    
    call ioda_vecstring_init(listed_groups_g4)	
    call g4%list(listed_groups_g4)
    if (listed_groups_g4%size() /= 2) then
        write(error_unit,*) 'Unexpected number of child groups for g3.'
        stop -1    
    end if         

    ! Open groups
    call g3%open(nsz,'g4',reopened_g3)

end program	
