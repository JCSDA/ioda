!
! (C) Copyright 2020-2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! 
! \file test-Engines-open.f90
! \brief fortran binding tests for ioda-engines engines.
!
! \author Patrick Nichols pnichols@ucar.edu
!
program test_engines_open
	use,intrinsic :: iso_fortran_env
	use :: ioda_engines_mod
	use :: ioda_groups_mod	
	type(ioda_group) :: g4
        character(len=*) :: fname = 'test=engines-3.hdf5' 
        call ioda_engines_hh_open_file(fname,1); 
end program
