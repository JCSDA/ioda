!
! (C) Copyright 2017-2019 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Define interface for C++ ObsSpace code called from Fortran

!-------------------------------------------------------------------------------
interface
!-------------------------------------------------------------------------------

type(c_ptr) function c_obsspace_construct(conf, tbegin, tend) bind(C, name='obsspace_construct_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: conf, tbegin, tend
end function c_obsspace_construct

subroutine c_obsspace_destruct(obss) bind(C, name='obsspace_destruct_f')
  use, intrinsic :: iso_c_binding
  implicit none
  type(c_ptr), value :: obss
end subroutine c_obsspace_destruct

type(c_ptr) function c_obsspace_obsvariables(obss) bind(C, name='obsspace_obsvariables_f')
  use, intrinsic :: iso_c_binding, only : c_ptr
  implicit none
  type(c_ptr), value :: obss
end function c_obsspace_obsvariables

integer(kind=c_size_t) function c_obsspace_get_gnlocs(obss) bind(C,name='obsspace_get_gnlocs_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
end function c_obsspace_get_gnlocs

integer(kind=c_size_t) function c_obsspace_get_nlocs(obss) bind(C,name='obsspace_get_nlocs_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
end function c_obsspace_get_nlocs

integer(kind=c_size_t) function c_obsspace_get_nchans(obss) bind(C,name='obsspace_get_nchans_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
end function c_obsspace_get_nchans

integer(kind=c_size_t) function c_obsspace_get_nrecs(obss) bind(C,name='obsspace_get_nrecs_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
end function c_obsspace_get_nrecs

integer(kind=c_size_t) function c_obsspace_get_nvars(obss) bind(C,name='obsspace_get_nvars_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
end function c_obsspace_get_nvars

subroutine c_obsspace_get_dim_name(obss, dim_id, len_dim_name, dim_name ) bind(C,name='obsspace_get_dim_name_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
  integer(c_int), intent(in) :: dim_id
  integer(c_size_t), intent(inout) :: len_dim_name
  character(kind=c_char, len=1), intent(inout) :: dim_name(*)
end subroutine c_obsspace_get_dim_name

integer(kind=c_size_t) function c_obsspace_get_dim_size(obss, dim_id) bind(C,name='obsspace_get_dim_size_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
  integer(c_int), intent(in) :: dim_id
end function c_obsspace_get_dim_size

integer(kind=c_int) function c_obsspace_get_dim_id(obss, dim_name) bind(C,name='obsspace_get_dim_id_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: dim_name(*)
end function c_obsspace_get_dim_id

subroutine c_obsspace_obsname(obss, lcname, cname) bind (C,name='obsspace_obsname_f')
  use, intrinsic :: iso_c_binding, only : c_ptr, c_char, c_size_t
  implicit none
  type(c_ptr), value :: obss
  integer(c_size_t),intent(inout) :: lcname
  character(kind=c_char,len=1), intent(inout) :: cname(*)
end subroutine c_obsspace_obsname

subroutine c_obsspace_get_comm(obss, lcname, cname) bind(C,name='obsspace_get_comm_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value :: obss
  integer(c_int),intent(inout) :: lcname                  !< Communicator name length
  character(kind=c_char,len=1), intent(inout) :: cname(*) !< Communicator name
end subroutine c_obsspace_get_comm

subroutine c_obsspace_get_recnum(obss, length, recnum) &
              & bind(C,name='obsspace_get_recnum_f')
  use, intrinsic :: iso_c_binding, only : c_ptr, c_size_t
  implicit none
  type(c_ptr), value               :: obss
  integer(c_size_t), intent(in)    :: length
  integer(c_size_t), intent(inout) :: recnum(length)
end subroutine c_obsspace_get_recnum

subroutine c_obsspace_get_index(obss, length, indx) &
              & bind(C,name='obsspace_get_index_f')
  use, intrinsic :: iso_c_binding, only : c_ptr, c_size_t
  implicit none
  type(c_ptr), value               :: obss
  integer(c_size_t), intent(in)    :: length
  integer(c_size_t), intent(inout) :: indx(length)
end subroutine c_obsspace_get_index

logical(kind=c_bool) function c_obsspace_has(obss, group, vname) bind(C,name='obsspace_has_f')
  use, intrinsic :: iso_c_binding
  implicit none

  type(c_ptr), value                        :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
end function c_obsspace_has

!-------------------------------------------------------------------------------
! get data from ObsSpace

subroutine c_obsspace_get_int32(obss, group, vname, length, vect, len_cs, chan_select) &
              & bind(C,name='obsspace_get_int32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_int,c_size_t,c_int32_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int32_t), intent(inout) :: vect(length)
  integer(c_size_t), intent(in) :: len_cs
  integer(c_int), intent(in) :: chan_select(len_cs)
end subroutine c_obsspace_get_int32

subroutine c_obsspace_get_int64(obss, group, vname, length, vect, len_cs, chan_select) &
              & bind(C,name='obsspace_get_int64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_int,c_size_t,c_int64_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int64_t), intent(inout) :: vect(length)
  integer(c_size_t), intent(in) :: len_cs
  integer(c_int), intent(in) :: chan_select(len_cs)
end subroutine c_obsspace_get_int64

subroutine c_obsspace_get_real32(obss, group, vname, length, vect, len_cs, chan_select) &
              & bind(C,name='obsspace_get_real32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_int,c_size_t,c_float
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_float), intent(inout) :: vect(length)
  integer(c_size_t), intent(in) :: len_cs
  integer(c_int), intent(in) :: chan_select(len_cs)
end subroutine c_obsspace_get_real32

subroutine c_obsspace_get_real64(obss, group, vname, length, vect, len_cs, chan_select) &
              & bind(C,name='obsspace_get_real64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_int,c_size_t,c_double
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_double), intent(inout) :: vect(length)
  integer(c_size_t), intent(in) :: len_cs
  integer(c_int), intent(in) :: chan_select(len_cs)
end subroutine c_obsspace_get_real64

subroutine c_obsspace_get_datetime(obss, group, vname, length, date, time, len_cs, chan_select) &
              & bind(C,name='obsspace_get_datetime_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_int,c_size_t,c_int32_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int32_t), intent(inout) :: date(length)
  integer(c_int32_t), intent(inout) :: time(length)
  integer(c_size_t), intent(in) :: len_cs
  integer(c_int), intent(in) :: chan_select(len_cs)
end subroutine c_obsspace_get_datetime

subroutine c_obsspace_get_window(obss, wnd_bgn, wnd_end) bind(C,name='obsspace_get_window_f')
  use, intrinsic :: iso_c_binding, only : c_ptr
  implicit none
  type(c_ptr), value :: obss
  type(c_ptr), value :: wnd_bgn
  type(c_ptr), value :: wnd_end
end subroutine c_obsspace_get_window

subroutine c_obsspace_get_bool(obss, group, vname, length, vect, len_cs, chan_select) &
              & bind(C,name='obsspace_get_bool_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_int,c_size_t,c_bool
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  logical(c_bool), intent(inout) :: vect(length)
  integer(c_size_t), intent(in) :: len_cs
  integer(c_int), intent(in) :: chan_select(len_cs)
end subroutine c_obsspace_get_bool

!-------------------------------------------------------------------------------
! store data in ObsSpace

subroutine c_obsspace_put_int32(obss, group, vname, length, vect, ndims, dim_ids) &
              & bind(C,name='obsspace_put_int32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int,c_int32_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int32_t), intent(in) :: vect(length)
  integer(c_size_t), intent(in) :: ndims
  integer(c_int), intent(in) :: dim_ids(ndims)
end subroutine c_obsspace_put_int32

subroutine c_obsspace_put_int64(obss, group, vname, length, vect, ndims, dim_ids) &
              & bind(C,name='obsspace_put_int64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int,c_int64_t
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  integer(c_int64_t), intent(in) :: vect(length)
  integer(c_size_t), intent(in) :: ndims
  integer(c_int), intent(in) :: dim_ids(ndims)
end subroutine c_obsspace_put_int64

subroutine c_obsspace_put_real32(obss, group, vname, length, vect, ndims, dim_ids) &
              & bind(C,name='obsspace_put_real32_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int,c_float
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_float), intent(in) :: vect(length)
  integer(c_size_t), intent(in) :: ndims
  integer(c_int), intent(in) :: dim_ids(ndims)
end subroutine c_obsspace_put_real32

subroutine c_obsspace_put_real64(obss, group, vname, length, vect, ndims, dim_ids) &
              & bind(C,name='obsspace_put_real64_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int,c_double
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  real(c_double), intent(in) :: vect(length)
  integer(c_size_t), intent(in) :: ndims
  integer(c_int), intent(in) :: dim_ids(ndims)
end subroutine c_obsspace_put_real64

subroutine c_obsspace_put_bool(obss, group, vname, length, vect, ndims, dim_ids) &
              & bind(C,name='obsspace_put_bool_f')
  use, intrinsic :: iso_c_binding, only : c_ptr,c_char,c_size_t,c_int,c_bool
  implicit none
  type(c_ptr), value :: obss
  character(kind=c_char, len=1), intent(in) :: group(*)
  character(kind=c_char, len=1), intent(in) :: vname(*)
  integer(c_size_t), intent(in) :: length
  logical(c_bool), intent(in) :: vect(length)
  integer(c_size_t), intent(in) :: ndims
  integer(c_int), intent(in) :: dim_ids(ndims)
end subroutine c_obsspace_put_bool

!-------------------------------------------------------------------------------

integer(kind=c_int) function c_obsspace_get_location_dim_id() bind(C,name='obsspace_get_location_dim_id_f')
  use, intrinsic :: iso_c_binding, only: c_int
  implicit none
end function c_obsspace_get_location_dim_id

integer(kind=c_int) function c_obsspace_get_channel_dim_id() bind(C,name='obsspace_get_channel_dim_id_f')
  use, intrinsic :: iso_c_binding, only: c_int
  implicit none
end function c_obsspace_get_channel_dim_id

!-------------------------------------------------------------------------------
end interface
!-------------------------------------------------------------------------------
