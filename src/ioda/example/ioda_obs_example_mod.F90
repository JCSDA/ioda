!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran example module for observation space

! TODO: replace example with your_obsspace_name through the file

module ioda_obs_example_mod

use kinds
use fckit_log_module, only : fckit_log

implicit none
private
integer, parameter :: max_string=800

public ioda_obs_example
public ioda_obs_example_setup, ioda_obs_example_delete
public ioda_obs_example_read, ioda_obs_example_generate

! ------------------------------------------------------------------------------

!> Fortran derived type to hold observation space info
! TODO: fill in, below is just an example
type :: ioda_obs_example
  integer :: nobs
end type ioda_obs_example

! ------------------------------------------------------------------------------

contains
! ------------------------------------------------------------------------------
! TODO: replace the below function with your constructor of obsspace
subroutine ioda_obs_example_setup(self, nobs)
implicit none
type(ioda_obs_example), intent(inout) :: self
integer, intent(in) :: nobs

call ioda_obs_example_delete(self)

self%nobs = nobs

end subroutine ioda_obs_example_setup

! ------------------------------------------------------------------------------
! TODO: replace the below function with your destructor of obsspace
subroutine ioda_obs_example_delete(self)
implicit none
type(ioda_obs_example), intent(inout) :: self

end subroutine ioda_obs_example_delete

! ------------------------------------------------------------------------------
! TODO: replace the below function with your random obs generator
subroutine ioda_obs_example_generate(self, nobs)
implicit none
type(ioda_obs_example), intent(inout) :: self
integer, intent(in) :: nobs

end subroutine ioda_obs_example_generate

! ------------------------------------------------------------------------------
! TODO: replace the below function with your obsspace read
subroutine ioda_obs_example_read(filename, self)
implicit none
character(max_string), intent(in)   :: filename
type(ioda_obs_example), intent(inout), target :: self

end subroutine ioda_obs_example_read

! ------------------------------------------------------------------------------

end module ioda_obs_example_mod
