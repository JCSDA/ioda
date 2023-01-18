!
! (C) Copyright 2017 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Fortran interface to ObsSpace.

module obsspace_mod

use, intrinsic :: iso_c_binding
use kinds
use string_f_c_mod
use datetime_mod

implicit none

private
public obsspace_construct
public obsspace_destruct
public obsspace_obsname
public obsspace_obsvariables
public obsspace_get_gnlocs
public obsspace_get_nlocs
public obsspace_get_nchans
public obsspace_get_nrecs
public obsspace_get_nvars
public obsspace_get_dim_name
public obsspace_get_dim_size
public obsspace_get_dim_id
public obsspace_get_comm
public obsspace_get_recnum
public obsspace_get_index
public obsspace_get_db
public obsspace_put_db
public obsspace_has
public obsspace_get_location_dim_id
public obsspace_get_channel_dim_id
public obsspace_get_window

#include "ioda/obsspace_interface.f"

!-------------------------------------------------------------------------------

interface obsspace_get_db
  module procedure obsspace_get_db_int32
  module procedure obsspace_get_db_int64
  module procedure obsspace_get_db_real32
  module procedure obsspace_get_db_real64
  module procedure obsspace_get_db_datetime
  module procedure obsspace_get_db_bool
end interface

interface obsspace_put_db
  module procedure obsspace_put_db_int32
  module procedure obsspace_put_db_int64
  module procedure obsspace_put_db_real32
  module procedure obsspace_put_db_real64
  module procedure obsspace_put_db_bool
end interface

!-------------------------------------------------------------------------------
contains
!-------------------------------------------------------------------------------

type(c_ptr) function obsspace_construct(c_conf, tbegin, tend)
  use fckit_configuration_module, only: fckit_configuration
  use datetime_mod, only: datetime
  implicit none
  type(fckit_configuration), intent(in) :: c_conf
  type(datetime), intent(in) :: tbegin, tend
  type(c_ptr) :: c_tbegin, c_tend

  call f_c_datetime(tbegin, c_tbegin)
  call f_c_datetime(tend, c_tend)
  obsspace_construct = c_obsspace_construct(c_conf%c_ptr(), c_tbegin, c_tend)
end function obsspace_construct

!-------------------------------------------------------------------------------

subroutine obsspace_destruct(c_obss)
  implicit none
  type(c_ptr), intent(inout) :: c_obss

  call c_obsspace_destruct(c_obss)
  c_obss = c_null_ptr
end subroutine obsspace_destruct

!-------------------------------------------------------------------------------

!> Get obsname from ObsSpace

subroutine obsspace_obsname(obss, obsname)
  use string_f_c_mod
  implicit none

  type(c_ptr), value, intent(in) :: obss
  character(*), intent(inout)    :: obsname

  integer(c_size_t) :: lcname

  !< If changing the length of name and cname, need to change the ASSERT in
  !obsspace_f.cc also
  character(kind=c_char,len=1) :: cname(101)

  call c_obsspace_obsname(obss, lcname, cname)
  call c_f_string(cname, obsname)
  obsname = obsname(1:lcname)

end subroutine obsspace_obsname

!-------------------------------------------------------------------------------

!> Get obsvariables from ObsSpace

type(oops_variables) function obsspace_obsvariables(obss)
  use oops_variables_mod
  implicit none
  type(c_ptr), value, intent(in) :: obss

  obsspace_obsvariables = oops_variables(c_obsspace_obsvariables(obss))
end function obsspace_obsvariables

!-------------------------------------------------------------------------------
!>  Return the number of observational locations in the input obs file

integer function obsspace_get_gnlocs(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_gnlocs = c_obsspace_get_gnlocs(c_obss)
end function obsspace_get_gnlocs

!-------------------------------------------------------------------------------
!>  Return the number of observational locations in the obs space

integer function obsspace_get_nlocs(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_nlocs = c_obsspace_get_nlocs(c_obss)
end function obsspace_get_nlocs

!-------------------------------------------------------------------------------
!>  Return the number of channels in obs space (zero if conventional obs type)

integer function obsspace_get_nchans(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_nchans = c_obsspace_get_nchans(c_obss)
end function obsspace_get_nchans

!-------------------------------------------------------------------------------
!>  Return the number of observational records (profiles)

integer function obsspace_get_nrecs(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_nrecs = c_obsspace_get_nrecs(c_obss)
end function obsspace_get_nrecs

!-------------------------------------------------------------------------------
!>  Return the number of observational variables

integer function obsspace_get_nvars(c_obss)
  implicit none
  type(c_ptr), intent(in) :: c_obss

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_nvars = c_obsspace_get_nvars(c_obss)
end function obsspace_get_nvars

!-------------------------------------------------------------------------------
!>  Return the ObsSpace dimension name given the dimension id

subroutine obsspace_get_dim_name(obss, dim_id, dim_name)
  use string_f_c_mod
  implicit none

  type(c_ptr), value, intent(in) :: obss
  integer, intent(in)            :: dim_id
  character(*), intent(inout)    :: dim_name

  integer(c_size_t) :: len_dim_name

  !< If changing the length of dim_name and c_dim_name, need to change the ASSERT in
  !obsspace_f.cc also
  character(kind=c_char,len=1) :: c_dim_name(101)

  call c_obsspace_get_dim_name(obss, dim_id, len_dim_name, c_dim_name)
  call c_f_string(c_dim_name, dim_name)
  dim_name = dim_name(1:len_dim_name)

end subroutine obsspace_get_dim_name

!-------------------------------------------------------------------------------
!>  Return the size of the ObsSpace dimension given the dimension id

integer function obsspace_get_dim_size(obss, dim_id)
  implicit none
  type(c_ptr), intent(in) :: obss
  integer, intent(in)     :: dim_id

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_dim_size = c_obsspace_get_dim_size(obss, dim_id)
end function obsspace_get_dim_size

!-------------------------------------------------------------------------------
!>  Return the ObsSpace dimension id given the dimension name

integer function obsspace_get_dim_id(obss, dim_name)
  use string_f_c_mod
  implicit none
  type(c_ptr), intent(in) :: obss
  character(len=*), intent(in) :: dim_name

  character(kind=c_char,len=1), allocatable :: c_dim_name(:)
  call f_c_string(dim_name, c_dim_name)

  ! Implicit conversion from c_size_t to integer which is safe in this case
  obsspace_get_dim_id = c_obsspace_get_dim_id(obss, c_dim_name)
end function obsspace_get_dim_id

!-------------------------------------------------------------------------------
!>  Return the name and name length of obsspace communicator
subroutine obsspace_get_comm(obss, f_comm)
  use fckit_mpi_module, only: fckit_mpi_comm
  use string_f_c_mod
  implicit none

  type(c_ptr), intent(in)          :: obss
  type(fckit_mpi_comm),intent(out) :: f_comm

  integer :: lcname
  !< If changing the length of name and cname, need to change the ASSERT in obsspace_f.cc also
  character(kind=c_char,len=1) :: cname(101)
  character(len=100) :: name
  character(len=:), allocatable :: name_comm

  call c_obsspace_get_comm(obss, lcname, cname)
  call c_f_string(cname, name)

  name_comm = name(1:lcname)
  f_comm = fckit_mpi_comm(name_comm)
end subroutine obsspace_get_comm

!-------------------------------------------------------------------------------
!>  Get the record number vector
subroutine obsspace_get_recnum(obss, recnum)
  implicit none
  type(c_ptr), intent(in)          :: obss
  integer(c_size_t), intent(inout) :: recnum(:)

  integer(c_size_t) :: length

  length = size(recnum)
  call c_obsspace_get_recnum(obss, length, recnum)
end subroutine obsspace_get_recnum

!-------------------------------------------------------------------------------
!>  Get the index vector
subroutine obsspace_get_index(obss, indx)
  implicit none
  type(c_ptr), intent(in)          :: obss
  integer(c_size_t), intent(inout) :: indx(:)

  integer(c_size_t) :: length

  length = size(indx)
  call c_obsspace_get_index(obss, length, indx)
end subroutine obsspace_get_index

!-------------------------------------------------------------------------------

!>  Return true if variable exists in database

logical function obsspace_has(c_obss, group, vname)
  implicit none
  type(c_ptr), intent(in)      :: c_obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)

  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  obsspace_has = c_obsspace_has(c_obss, c_group, c_vname)
end function obsspace_has

!-------------------------------------------------------------------------------

!> Get a variable from the ObsSpace database

subroutine obsspace_get_db_int32(obss, group, vname, vect, chan_select)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int32_t), intent(inout) :: vect(:)
  integer(c_int), intent(in), optional :: chan_select(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: len_cs
  integer(c_int)    :: dummy_chan_select(1)  ! used as a fallback if chan_select is not present

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)
  if (present(chan_select)) then
    len_cs = size(chan_select)
    call c_obsspace_get_int32(obss, c_group, c_vname, length, vect, len_cs, chan_select)
  else
    ! Note: we say that the number of channels is zero, which means that the contents of
    ! dummy_chan_select won't actually be accessed.
    call c_obsspace_get_int32(obss, c_group, c_vname, length, vect, 0_c_size_t, dummy_chan_select)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_int32


!-------------------------------------------------------------------------------

!> Get a variable from the ObsSpace database

subroutine obsspace_get_db_int64(obss, group, vname, vect, chan_select)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int64_t), intent(inout) :: vect(:)
  integer(c_int), intent(in), optional :: chan_select(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: len_cs
  integer(c_int)    :: dummy_chan_select(1)  ! used as a fallback if chan_select is not present

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)
  if (present(chan_select)) then
    len_cs = size(chan_select)
    call c_obsspace_get_int64(obss, c_group, c_vname, length, vect, len_cs, chan_select)
  else
    ! Note: we say that the number of channels is zero, which means that the contents of
    ! dummy_chan_select won't actually be accessed.
    call c_obsspace_get_int64(obss, c_group, c_vname, length, vect, 0_c_size_t, dummy_chan_select)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_int64

!-------------------------------------------------------------------------------

!> Get a variable from the ObsSpace database

subroutine obsspace_get_db_real32(obss, group, vname, vect, chan_select)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_float), intent(inout) :: vect(:)
  integer(c_int), intent(in), optional :: chan_select(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: len_cs
  integer(c_int)    :: dummy_chan_select(1)  ! used as a fallback if chan_select is not present

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)
  if (present(chan_select)) then
    len_cs = size(chan_select)
    call c_obsspace_get_real32(obss, c_group, c_vname, length, vect, len_cs, chan_select)
  else
    ! Note: we say that the number of channels is zero, which means that the contents of
    ! dummy_chan_select won't actually be accessed.
    call c_obsspace_get_real32(obss, c_group, c_vname, length, vect, 0_c_size_t, dummy_chan_select)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_real32

!-------------------------------------------------------------------------------

!> Get a variable from the ObsSpace database

subroutine obsspace_get_db_real64(obss, group, vname, vect, chan_select)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_double), intent(inout) :: vect(:)
  ! optional; if omitted, all channels will be retrieved
  integer(c_int), intent(in), optional :: chan_select(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: len_cs
  integer(c_int)    :: dummy_chan_select(1)  ! used as a fallback if chan_select is not present

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  if (present(chan_select)) then
    len_cs = size(chan_select)
    call c_obsspace_get_real64(obss, c_group, c_vname, length, vect, len_cs, chan_select)
  else
    ! Note: we say that the number of channels is zero, which means that the contents of
    ! dummy_chan_select won't actually be accessed.
    call c_obsspace_get_real64(obss, c_group, c_vname, length, vect, 0_c_size_t, dummy_chan_select)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_real64

!-------------------------------------------------------------------------------

!> Get datetime from the ObsSpace database

subroutine obsspace_get_db_datetime(obss, group, vname, vect, chan_select)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  type(datetime), intent(inout) :: vect(:)
  integer(c_int), intent(in), optional :: chan_select(:)

  integer(c_size_t) :: length, i
  integer(c_size_t) :: len_cs
  integer(c_int)    :: dummy_chan_select(1)  ! used as a fallback if chan_select is not present
  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_int32_t), dimension(:), allocatable :: date
  integer(c_int32_t), dimension(:), allocatable :: time
  character(len=20) :: fstring

  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  allocate(date(length), time(length))

  if (present(chan_select)) then
    len_cs = size(chan_select)
    call c_obsspace_get_datetime(obss, c_group, c_vname, length, date, time, len_cs, chan_select)
  else
    ! Note: we say that the number of channels is zero, which means that the contents of
    ! dummy_chan_select won't actually be accessed.
    call c_obsspace_get_datetime(obss, c_group, c_vname, length, date, time, &
                                 0_c_size_t, dummy_chan_select)
  endif

  ! Constrct datatime based on date and time
  do i = 1, length
    write(fstring, "(i4.4, a, i2.2, a, i2.2, a, i2.2, a, i2.2, a, i2.2, a)") &
          date(i)/10000, '-', MOD(date(i), 10000)/100, '-', MOD(MOD(date(i), 10000), 100), 'T', &
          time(i)/10000, ':', MOD(time(i), 10000)/100, ':', MOD(MOD(time(i), 10000), 100), 'Z'
    call datetime_create(fstring, vect(i))
  enddo

  ! Clean up
  deallocate(date, time)
end subroutine obsspace_get_db_datetime

!-------------------------------------------------------------------------------

!>  Return the DA timing window (start, end)

subroutine obsspace_get_window(obss, tbegin, tend)
  use datetime_mod, only: datetime
  implicit none
  type(c_ptr), intent(in)     :: obss
  type(datetime), intent(out) :: tbegin
  type(datetime), intent(out) :: tend

  type(c_ptr) :: c_tbegin, c_tend

  ! Initialize
  call datetime_create("0001-01-01T01:01:01Z", tbegin)
  call datetime_create("0001-01-01T01:01:01Z", tend)

  ! Copy to c++
  call f_c_datetime(tbegin, c_tbegin)
  call f_c_datetime(tend, c_tend)

  ! Copy correct value into c++ object
  call c_obsspace_get_window(obss, c_tbegin, c_tend)

  ! Copy back to Fortran
  call c_f_datetime(c_tbegin, tbegin)
  call c_f_datetime(c_tend, tend)

end subroutine obsspace_get_window

!-------------------------------------------------------------------------------

!> Get a variable from the ObsSpace database

subroutine obsspace_get_db_bool(obss, group, vname, vect, chan_select)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  logical(c_bool), intent(inout) :: vect(:)
  integer(c_int), intent(in), optional :: chan_select(:)

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: len_cs
  integer(c_int)    :: dummy_chan_select(1)  ! used as a fallback if chan_select is not present

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)
  if (present(chan_select)) then
    len_cs = size(chan_select)
    call c_obsspace_get_bool(obss, c_group, c_vname, length, vect, len_cs, chan_select)
  else
    ! Note: we say that the number of channels is zero, which means that the contents of
    ! dummy_chan_select won't actually be accessed.
    call c_obsspace_get_bool(obss, c_group, c_vname, length, vect, 0_c_size_t, dummy_chan_select)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_get_db_bool

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_int32(obss, group, vname, vect, dim_ids)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int32_t), intent(in) :: vect(:)
  integer(c_int), intent(in), optional :: dim_ids(:)  ! if not set, defaults to Location

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: ndims
  integer(c_int) :: fallback_dim_ids(1)

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  if (present(dim_ids)) then
    ndims = size(dim_ids)
    call c_obsspace_put_int32(obss, c_group, c_vname, length, vect, ndims, dim_ids)
  else
    ndims = 1
    fallback_dim_ids = (/ obsspace_get_location_dim_id() /)
    call c_obsspace_put_int32(obss, c_group, c_vname, length, vect, ndims, fallback_dim_ids)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_int32

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_int64(obss, group, vname, vect, dim_ids)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  integer(c_int64_t), intent(in) :: vect(:)
  integer(c_int), intent(in), optional :: dim_ids(:)  ! if not set, defaults to Location

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: ndims
  integer(c_int) :: fallback_dim_ids(1)

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  if (present(dim_ids)) then
    ndims = size(dim_ids)
    call c_obsspace_put_int64(obss, c_group, c_vname, length, vect, ndims, dim_ids)
  else
    ndims = 1
    fallback_dim_ids = (/ obsspace_get_location_dim_id() /)
    call c_obsspace_put_int64(obss, c_group, c_vname, length, vect, ndims, fallback_dim_ids)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_int64

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_real32(obss, group, vname, vect, dim_ids)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_float), intent(in) :: vect(:)
  integer(c_int), intent(in), optional :: dim_ids(:)  ! if not set, defaults to Location

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: ndims
  integer(c_int) :: fallback_dim_ids(1)

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  if (present(dim_ids)) then
    ndims = size(dim_ids)
    call c_obsspace_put_real32(obss, c_group, c_vname, length, vect, ndims, dim_ids)
  else
    ndims = 1
    fallback_dim_ids = (/ obsspace_get_location_dim_id() /)
    call c_obsspace_put_real32(obss, c_group, c_vname, length, vect, ndims, fallback_dim_ids)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_real32

!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_real64(obss, group, vname, vect, dim_ids)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  real(c_double), intent(in) :: vect(:)
  integer(c_int), intent(in), optional :: dim_ids(:)  ! if not set, defaults to Location

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: ndims
  integer(c_int) :: fallback_dim_ids(1)

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  if (present(dim_ids)) then
    ndims = size(dim_ids)
    call c_obsspace_put_real64(obss, c_group, c_vname, length, vect, ndims, dim_ids)
  else
    ndims = 1
    fallback_dim_ids = (/ obsspace_get_location_dim_id() /)
    call c_obsspace_put_real64(obss, c_group, c_vname, length, vect, ndims, fallback_dim_ids)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_real64
!-------------------------------------------------------------------------------

!>  Store a vector in ObsSpace database

subroutine obsspace_put_db_bool(obss, group, vname, vect, dim_ids)
  implicit none
  type(c_ptr), value, intent(in) :: obss
  character(len=*), intent(in) :: group
  character(len=*), intent(in) :: vname
  logical(c_bool), intent(in) :: vect(:)
  integer(c_int), intent(in), optional :: dim_ids(:)  ! if not set, defaults to Location

  character(kind=c_char,len=1), allocatable :: c_group(:), c_vname(:)
  integer(c_size_t) :: length
  integer(c_size_t) :: ndims
  integer(c_int) :: fallback_dim_ids(1)

  !  Translate query from Fortran string to C++ char[].
  call f_c_string(group, c_group)
  call f_c_string(vname, c_vname)
  length = size(vect)

  if (present(dim_ids)) then
    ndims = size(dim_ids)
    call c_obsspace_put_bool(obss, c_group, c_vname, length, vect, ndims, dim_ids)
  else
    ndims = 1
    fallback_dim_ids = (/ obsspace_get_location_dim_id() /)
    call c_obsspace_put_bool(obss, c_group, c_vname, length, vect, ndims, fallback_dim_ids)
  endif

  deallocate(c_group, c_vname)
end subroutine obsspace_put_db_bool

!-------------------------------------------------------------------------------

!>  Return the identifier of the Location dimension.
integer(c_int) function obsspace_get_location_dim_id()
  implicit none
  obsspace_get_location_dim_id = c_obsspace_get_location_dim_id()
end function obsspace_get_location_dim_id

!-------------------------------------------------------------------------------

!>  Return the identifier of the Channel dimension.
integer(c_int) function obsspace_get_channel_dim_id()
  implicit none
  obsspace_get_channel_dim_id = c_obsspace_get_channel_dim_id()
end function obsspace_get_channel_dim_id

end module obsspace_mod
