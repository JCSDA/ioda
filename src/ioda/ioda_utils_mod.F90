!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

!> Fortran module containing IODA utility programs

module ioda_utils_mod

  use kinds, only: kind_real

  implicit none

  private

  public missing_value
  public ioda_deselect_missing_values

  !======================================================
  ! Constants
  !======================================================
  real(kind=kind_real) :: missing_value = 9.0e36_kind_real !< default for NetCDF file converted from PrepBUFR, the actual missing value is 9.96921e+36

  !======================================================
  ! Subroutines
  !======================================================
  contains

  !---------------------------------------------------------------------------------------
  subroutine ioda_deselect_missing_values(ncid, vname, in_index, out_index)
    use netcdf, only: NF90_FLOAT, NF90_DOUBLE

    use nc_diag_read_mod, only: nc_diag_read_get_var
    use nc_diag_read_mod, only: nc_diag_read_get_var_dims
    use nc_diag_read_mod, only: nc_diag_read_get_var_type

    implicit none

    integer :: ncid                                               !< netcdf file id
    character(len=*) :: vname                                     !< netcdf variable name
    integer, dimension(:), intent(in) :: in_index                 !< vector of selection indices
    integer, dimension(:), allocatable, intent(out) :: out_index  !< output vector of selection indices

    integer :: ndim
    integer, dimension(:), allocatable :: dims
    integer :: vtype
    real, dimension(:), allocatable :: var1d
    real, dimension(:,:), allocatable :: var2d
    real(kind_real), dimension(:), allocatable :: var1d_dbl
    real(kind_real), dimension(:,:), allocatable :: var2d_dbl

    integer :: i
    integer :: mcount
    integer :: mlen
    integer :: imiss


    ! Read in the test variable and check for missing values.
    if (allocated(dims)) deallocate(dims)
    call nc_diag_read_get_var_dims(ncid, vname, ndim, dims)
    vtype = nc_diag_read_get_var_type(ncid, vname)

    if (ndim == 1) then
      allocate(var1d(dims(1)))
      if (vtype == NF90_FLOAT) then
        call nc_diag_read_get_var(ncid, vname, var1d)
      elseif (vtype == NF90_DOUBLE) then
        allocate(var1d_dbl(dims(1)))
        call nc_diag_read_get_var(ncid, vname, var1d_dbl)
        var1d = real(var1d_dbl)
        deallocate(var1d_dbl)
      endif
    else
      allocate(var2d(dims(1), dims(2)))
      if (vtype == NF90_FLOAT) then
        call nc_diag_read_get_var(ncid, vname, var2d)
      elseif (vtype == NF90_DOUBLE) then
        allocate(var2d_dbl(dims(1), dims(2)))
        call nc_diag_read_get_var(ncid, vname, var2d_dbl)
        var2d = real(var2d_dbl)
        deallocate(var2d_dbl)
      endif

      allocate(var1d(dims(2)))
      var1d = var2d(1,:)
      deallocate(var2d)
    endif

    ! At this point, var1d contains a sample vector that can be checked
    ! for missing values. 

    ! Read the vector and count up the missing values so that a size
    ! can be determined for the output indx vector.
    mcount = 0
    do i = 1, size(in_index)
      if (var1d(in_index(i)) .ge. missing_value) then
        mcount = mcount + 1
      endif
    enddo
    mlen = size(in_index) - mcount

    ! Allocate the output index vector and go through the test vector again
    ! to determine which indices to omit (due to missing values).
    allocate(out_index(mlen))

    imiss = 0
    do i = 1, size(in_index)
      if (var1d(in_index(i)) .lt. missing_value) then
        imiss = imiss + 1
        out_index(imiss) = in_index(i)
      endif
    enddo

    deallocate(var1d)
    return
  endsubroutine ioda_deselect_missing_values

end module ioda_utils_mod
