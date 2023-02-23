!!!/*
!!! * (C) Copyright 2020-2022 UCAR
!!! *
!!! * This software is licensed under the terms of the Apache Licence Version 2.0
!!! * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
!!! */
!!!/** \file test-Engines.f90
!!! * \brief fortran binding tests for ioda-engines engines.
!!! *
!!! * \author Patrick Nichols pnichols@ucar.edu
!!! **/
program test_engines
	use,intrinsic :: iso_fortran_env
	use :: ioda_engines_mod
	use :: ioda_groups_mod	
	type(ioda_group) :: g1,g2,g3
	character(len=*) :: n1 = '1'
	character(len=*) :: n2 = '2'
        character(len=*) :: fname = 'test=engines-3.hdf5' 
        integer(int64) :: sz

        sz = 1000000

        call= ioda_engines_create_root_group(g1)
        call ioda_engines_hh_create_memory_file(g2,n1,sz)
	call ioda_engines_hh_create_file(g3,fname,1)
        call ioda_group_dtor(g3)  
        call ioda_group_dtor(g2)
        call ioda_group_dtor(g1)
end program

