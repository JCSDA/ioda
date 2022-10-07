!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Group manipulation using the Fortran interface. This example parallels the C++ example.

program ioda_fortran_01_groupsandobsspaces
    use, intrinsic :: iso_c_binding
    use :: ioda_f
    use :: ioda_vecstring_f
    use :: ioda_group_f
    use :: ioda_engines_f
    implicit none

    type(ioda_engines) :: engines
    type(ioda_group) :: grpFromFile, g1, g2, g3, g4, g5, g6, g8, reopened_g3
    type(ioda_vecstring) :: listed_groups

    ! Create a file
    !grpFromFile = engines%obsstore%createRootGroup()
    grpFromFile = engines%hh%createFile("Example-01-F.hdf5", ioda_Engines_BackendCreateModes_Truncate_If_Exists)

    ! Create groups in the file
    g1 = grpFromFile%create('g1')
    g2 = grpFromFile%create('g2')

    g3 = g1%create('g3')
    g4 = g3%create('g4')
    g5 = g4%create('g5')
    g6 = g4%create('g6')
    g8 = g6%create('g7/g8')

    ! Check for existence of groups
    if (g1%exists('g3') .eqv. .false.) then
        stop '/g1/g3 does not exist'
    end if

    if (g1%exists('g3/g4') .eqv. .false.) then
        stop '/g1/g3/g4 does not exist'
    end if

    ! Listing groups contained within a group. See VecString example for full usage.
    listed_groups = g3%list()
    if (listed_groups%size() /= 1) then
        stop 'Unexpected number of child groups.'
    end if

    ! Open groups
    reopened_g3 = g1%open('g3')

    listed_groups = reopened_g3%list()
    if (listed_groups%size() /= 1) then
        stop 'Unexpected number of child groups.'
    end if

    ! Done!
end program ioda_fortran_01_groupsandobsspaces
