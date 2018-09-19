! (C) Copyright 2018 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!> Fortran module to handle radiosonde observations

module ioda_obsdb_mod_c

use iso_c_binding
use string_f_c_mod
use config_mod
use datetime_mod
use duration_mod
use ioda_obsdb_mod
use ioda_locs_mod
use ioda_locs_mod_c, only : ioda_locs_registry
use ioda_obs_vectors
use type_distribution, only: random_distribution
use fckit_log_module, only : fckit_log
use fckit_mpi_module
use kinds
#ifdef HAVE_ODB_API
use odb_helper_mod, only: &
  count_query_results
#endif

#ifdef HAVE_ODB
use odb, only: &
  odb_cancel,  &
  odb_close,   &
  odb_open,    &
  odb_select

use odbgetput, only: &
  odb_get,           &
  odb_addview
#endif

implicit none
private

public :: ioda_obsdb_registry

! ------------------------------------------------------------------------------
integer, parameter :: max_string=800
! ------------------------------------------------------------------------------

#define LISTED_TYPE ioda_obsdb

!> Linked list interface - defines registry_t type
#include "linkedList_i.f"

!> Global registry
type(registry_t) :: ioda_obsdb_registry

! ------------------------------------------------------------------------------
contains
! ------------------------------------------------------------------------------
!> Linked list implementation
#include "linkedList_c.f"

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_setup_c(c_key_self, c_conf) bind(c,name='ioda_obsdb_setup_f90')

#ifdef HAVE_ODB_API
use odb_helper_mod, only: &
  count_query_results
#endif

use nc_diag_read_mod, only: nc_diag_read_get_dim
use nc_diag_read_mod, only: nc_diag_read_init
use nc_diag_read_mod, only: nc_diag_read_close

use ioda_utils_mod, only: ioda_deselect_missing_values

implicit none

integer(c_int), intent(inout) :: c_key_self
type(c_ptr), intent(in)       :: c_conf !< configuration

type(ioda_obsdb), pointer :: self
character(len=max_string) :: fin
character(len=max_string) :: fout
character(len=max_string) :: cfg_fout
logical                   :: fout_exists
type(fckit_mpi_comm)      :: comm
character(len=10)         :: cproc
integer                   :: ppos
character(len=max_string) :: MyObsType
character(len=255) :: record
integer :: fvlen
integer, allocatable :: dist_indx(:)
integer, allocatable :: miss_indx(:)
integer :: nobs
integer :: nlocs
integer :: nvars
integer :: iunit
type(random_distribution) :: ran_dist
integer :: ds, n, nn
integer :: input_file_type

#ifdef HAVE_ODB
integer :: rc, handle, num_pools, num_rows, num_cols
interface
  function setenv (envname, envval, overwrite) bind (c, name = "setenv")
    use, intrinsic :: iso_c_binding, only: &
      c_char,                              &
      c_int
    character(kind=c_char)     :: envname(*)
    character(kind=c_char)     :: envval(*)
    integer(kind=c_int), value :: overwrite
    integer(kind=c_int)        :: setenv
  end function setenv
end interface
#endif

! Get the obs type
MyObsType = trim(config_get_string(c_conf,max_string,"ObsType"))

if (config_element_exists(c_conf,"ObsData.ObsDataIn")) then
  fin  = config_get_string(c_conf,max_string,"ObsData.ObsDataIn.obsfile")
  if (fin(len_trim(fin)-3:len_trim(fin)) == ".nc4" .or. &
      fin(len_trim(fin)-3:len_trim(fin)) == ".nc") then
    input_file_type = 0
  else if (fin(len_trim(fin)-3:len_trim(fin)) == ".odb") then
    input_file_type = 1
  else
    input_file_type = 2
  end if

  ! For now, just reading in one variable, will read this from the input file eventually.
  !
  ! For radiance, there are 12 channels of data (channels 7, 8 and 14 were not assimilated
  ! into the GSI run for April 15, 2018 00Z). The brightness_temperature obs data is actually
  ! not used at this point. However, the CRTM always creates an hofx with 15 channels, and it
  ! needs the nobs value to be set as if all 15 channels of the obs data are present. We still
  ! need to decide how to handle missing channels in obs data, so to get things going for
  ! now set nvars to 15 for Radiance. Note that when we come to the point where we do want
  ! to read in the brightness temperature, we will need to address how to handle missing
  ! channels.
  nvars = 1 
  if (trim(MyObsType) .eq. "Radiance") nvars = 15

  select case (input_file_type)
    case (0)

      call nc_diag_read_init(fin, iunit)

      ! Get the length of the vectors in the input file
      if ((trim(MyObsType) .eq. "Radiance") .or. &
          (trim(MyObsType) .eq. "Radiosonde") .or. &
          (trim(MyObsType) .eq. "Aircraft")) then
        fvlen = nc_diag_read_get_dim(iunit, 'nlocs')
      else
        fvlen = nc_diag_read_get_dim(iunit, 'nobs')
      endif

      ! Apply the random distribution, which yields the size and indices that
      ! are to be selected by this process element out of the file.
      ran_dist = random_distribution(fvlen)
      if ((trim(MyObsType) .eq. "Radiance") .or. &
          (trim(MyObsType) .eq. "Radiosonde") .or. &
          (trim(MyObsType) .eq. "Aircraft")) then
        nlocs = ran_dist%nobs_pe()
        allocate(dist_indx(nlocs))
        dist_indx = ran_dist%indx

        nobs = nvars * nlocs
      else
        nobs = ran_dist%nobs_pe()
        allocate(dist_indx(nobs))
        dist_indx = ran_dist%indx

        nlocs = nobs
      endif

      ! Read in a variable and check for missing values. Adjust the nobs, nlocs values
      ! and dist_index accordingly.
      if ((trim(MyObsType) .eq. "Radiosonde") .or. &
          (trim(MyObsType) .eq. "Aircraft")) then
        call ioda_deselect_missing_values(iunit, "air_temperature", dist_indx, miss_indx)

        ! Reallocate dist_indx and copy miss_indx in. This should skip over missing values
        ! from the file.
        nlocs = size(miss_indx)
        deallocate(dist_indx)
        allocate(dist_indx(nlocs))
        dist_indx = miss_indx

        nobs = nvars * nlocs

        deallocate(miss_indx)
      endif

      call nc_diag_read_close(fin)

    case (1)

#ifdef HAVE_ODB_API

      call count_query_results (fin, 'select seqno from "' // trim(fin) // '" where entryno = 1;', nobs)
      fvlen = nobs
      nlocs = nobs
      allocate (dist_indx(nobs))
      dist_indx(:) = [(/(n, n = 1,nobs)/)]

#endif

    case (2)

#ifdef HAVE_ODB

      rc = setenv ("ODB_SRCPATH_OOPS" // c_null_char, trim (fin) // c_null_char, 1_c_int)
      rc = setenv ("ODB_DATAPATH_OOPS" // c_null_char, trim (fin) // c_null_char, 1_c_int)
      rc = setenv ("IOASSIGN" // c_null_char, trim (fin) // '/IOASSIGN' // c_null_char, 1_c_int)
      handle = odb_open ("OOPS", "OLD", num_pools)
      rc = odb_addview (handle, "query", abort = .false.)
      rc = odb_select (handle, "query", num_rows, num_cols)
      allocate (self % odb_data(num_rows,0:num_cols))
      rc = odb_get (handle, "query", self % odb_data, num_rows)
      rc = odb_cancel (handle, "query")
      rc = odb_close (handle)
      nobs = size(self % odb_data,dim=1)
      fvlen = nobs
      nlocs = nobs
      allocate (dist_indx(nobs))
      dist_indx(:) = [(/(n, n = 1,nobs)/)]

#endif

  end select

else
  fin  = ""
  fvlen = 0
  nobs = 0
  nlocs = 0
  ! Create a dummy index array
  allocate(dist_indx(1))
  dist_indx(1) = -1
endif

write(record,*) 'ioda_obsdb_setup_c: ', trim(MyObsType), ' file in =',trim(fin)
call fckit_log%info(record)

! Check to see if an output file has been requested.
if (config_element_exists(c_conf,"ObsData.ObsDataOut")) then
   cfg_fout = config_get_string(c_conf,max_string,"ObsData.ObsDataOut.obsfile")

   ! Tag the process rank onto the end of the file name so that multi processes won't
   ! collide with each other. Place the process rank number right before the file
   ! extension.

   ! get the process rank number
   comm = fckit_mpi_comm()
   write(cproc,fmt='(i4.4)') comm%rank()

   ! Find the left-most dot in the file name, and use that to pick off the file name
   ! and file extension.
   ppos = scan(trim(cfg_fout), '.', BACK=.true.)
   if (ppos > 0) then
      ! found a file extension
      fout = cfg_fout(1:ppos-1) // '_' // trim(adjustl(cproc)) // trim(cfg_fout(ppos:))
   else
      ! no file extension
      fout = trim(cfg_fout) // '_' // trim(adjustl(cproc))
   endif 

   ! Check to see if user is trying to overwrite an existing file. For now always allow 
   ! the overwrite, but issue a warning if we are about to clobber an existing file.
   inquire(file=trim(fout), exist=fout_exists)
   if (fout_exists) then
      write(record,*) 'ioda_obsdb_setup_c: WARNING: Overwriting output file: ', trim(fout)
      call fckit_log%info(record)
   endif

endif


call ioda_obsdb_registry%init()
call ioda_obsdb_registry%add(c_key_self)
call ioda_obsdb_registry%get(c_key_self, self)

call ioda_obsdb_setup(self, fvlen, nobs, dist_indx, nlocs, nvars, fin, fout, MyObsType)

end subroutine ioda_obsdb_setup_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_delete_c(c_key_self) bind(c,name='ioda_obsdb_delete_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(ioda_obsdb), pointer :: self

call ioda_obsdb_registry%get(c_key_self, self)
call ioda_obsdb_delete(self)
call ioda_obsdb_registry%remove(c_key_self)

end subroutine ioda_obsdb_delete_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_nobs_c(c_key_self, kobs) bind(c,name='ioda_obsdb_nobs_f90')
implicit none
integer(c_int), intent(in) :: c_key_self
integer(c_int), intent(inout) :: kobs
type(ioda_obsdb), pointer :: self

call ioda_obsdb_registry%get(c_key_self, self)

kobs = self%nobs

end subroutine ioda_obsdb_nobs_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_getlocations_c(c_key_self, c_t1, c_t2, c_key_locs) bind(c,name='ioda_obsdb_getlocations_f90')
implicit none
integer(c_int), intent(in)    :: c_key_self
type(c_ptr), intent(in)       :: c_t1, c_t2
integer(c_int), intent(inout) :: c_key_locs

type(ioda_obsdb), pointer :: self
type(datetime) :: t1, t2
type(ioda_locs), pointer :: locs

call ioda_obsdb_registry%get(c_key_self, self)
call c_f_datetime(c_t1, t1)
call c_f_datetime(c_t2, t2)

call ioda_locs_registry%init()
call ioda_locs_registry%add(c_key_locs)
call ioda_locs_registry%get(c_key_locs,locs)

call ioda_obsdb_getlocs(self, locs)

end subroutine ioda_obsdb_getlocations_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_generate_c(c_key_self, c_conf, c_t1, c_t2) bind(c,name='ioda_obsdb_generate_f90')
implicit none
integer(c_int), intent(inout) :: c_key_self
type(c_ptr), intent(in)       :: c_conf !< configuration
type(c_ptr), intent(in)       :: c_t1, c_t2

type(ioda_obsdb), pointer :: self
type(datetime) :: t1, t2
integer :: fvlen
integer :: nobs
integer,allocatable :: dist_indx(:)
integer :: nlocs
integer :: nvars
character(len=max_string)  :: MyObsType
character(len=255) :: record
real :: lat, lon1, lon2
type(random_distribution) :: ran_dist

call ioda_obsdb_registry%get(c_key_self, self)
call c_f_datetime(c_t1, t1)
call c_f_datetime(c_t2, t2)

fvlen = config_get_int(c_conf, "nobs")
lat  = config_get_real(c_conf, "lat")
lon1 = config_get_real(c_conf, "lon1")
lon2 = config_get_real(c_conf, "lon2")

! Apply the random distribution, which yeilds nobs and the indices for selecting
! observations out of the file.
ran_dist = random_distribution(fvlen)
nobs = ran_dist%nobs_pe()
allocate(dist_indx(nobs))
dist_indx = ran_dist%indx

! For now, set fvlen and nlocs equal to nobs. This may need to change for some obs types.
nlocs = nobs

! For now, set nvars to one.
nvars = 1

! Record obs type
MyObsType = trim(config_get_string(c_conf,max_string,"ObsType"))
write(record,*) 'ioda_obsdb_generate_c: ', trim(MyObsType)
call fckit_log%info(record)

call ioda_obsdb_generate(self, fvlen, nobs, dist_indx, nlocs, nvars, MyObsType, lat, lon1, lon2)

deallocate(dist_indx)

end subroutine ioda_obsdb_generate_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_get_c(c_key_self, lcol, c_col, c_key_ovec) bind(c,name='ioda_obsdb_get_f90')  
implicit none
integer(c_int), intent(in) :: c_key_self
integer(c_int), intent(in) :: lcol
character(kind=c_char,len=1), intent(in) :: c_col(lcol+1)
integer(c_int), intent(in) :: c_key_ovec

type(ioda_obsdb), pointer :: self
type(obs_vector), pointer :: ovec

character(len=lcol) :: vname
integer :: i

type(obs_vector) :: TmpOvec

call ioda_obsdb_registry%get(c_key_self, self)
call ioda_obs_vect_registry%get(c_key_ovec,ovec)

ovec%nobs = self%nobs
! Copy C character array to Fortran string
do i = 1, lcol
  vname(i:i) = c_col(i)
enddo

! Quick hack for dealing with inverted observation error values in the netcdf
! file. Need to revisit this in the future and come up with a better
! solution.
if (trim(vname) .eq. "ObsErr") then
  call ioda_obsvec_setup(TmpOvec, self%nobs)
  call ioda_obsdb_var_to_ovec(self, TmpOvec, "Errinv_Input")
  ovec%values = 1.0_kind_real / TmpOvec%values
  call ioda_obsvec_delete(TmpOvec)
else
  call ioda_obsvec_setup(TmpOvec, self%nobs)
  call ioda_obsdb_var_to_ovec(self, TmpOvec, vname)
  ovec%values = TmpOvec%values
  call ioda_obsvec_delete(TmpOvec)
endif

end subroutine ioda_obsdb_get_c

! ------------------------------------------------------------------------------

subroutine ioda_obsdb_put_c(c_key_self, lcol, c_col, c_key_ovec) bind(c,name='ioda_obsdb_put_f90')
implicit none
integer(c_int), intent(in) :: c_key_self
integer(c_int), intent(in) :: lcol
character(kind=c_char,len=1), intent(in) :: c_col(lcol+1)
integer(c_int), intent(in) :: c_key_ovec

type(ioda_obsdb), pointer :: self
type(obs_vector), pointer :: ovec

character(len=lcol) :: vname
integer             :: i

call ioda_obsdb_registry%get(c_key_self, self)
call ioda_obs_vect_registry%get(c_key_ovec,ovec)

ovec%nobs = self%nobs
! Copy C character array to Fortran string
do i = 1, lcol
  vname(i:i) = c_col(i)
enddo

call ioda_obsdb_putvar(self, vname, ovec)

end subroutine ioda_obsdb_put_c

end module ioda_obsdb_mod_c
