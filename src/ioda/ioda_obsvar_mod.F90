!
! (C) Copyright 2017 UCAR
! 
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 

! IODA observation memory store
!
! For now using a linked list. This assumes that we have a small number of
! variables thus searching the list is fast. This implementation is meant to
! be a quick and dirty placeholder until a decision about ODB is made. We will
! then either replace this with ODB, or work on a more appropriate storage
! mechanism.

module ioda_obsvar_mod

use kinds

implicit none

integer, parameter :: IODA_OBSVAR_MAXSTRLEN = 80

!> observation variable
type :: ioda_obs_var
  integer :: nobs      !< number of observations

  character(len=IODA_OBSVAR_MAXSTRLEN) :: vname         !< variable name
  real(kind_real), allocatable :: vals(:)  !< values (nobs)

  type (ioda_obs_var), pointer :: next
end type ioda_obs_var

type :: ioda_obs_variables
  integer ::  n_nodes
  type (ioda_obs_var), pointer :: head

contains
  procedure :: setup => setup_
  procedure :: delete => delete_
  procedure :: add_node => add_node_
  procedure :: get_node => get_node_
  procedure :: remove_node => remove_node_
end type ioda_obs_variables

contains

!> Initialize the linked list
subroutine setup_(self)
  implicit none
  class (ioda_obs_variables) :: self

  ! Set the count to zero and nullify the head
  self%n_nodes = 0
  self%head => NULL()
end subroutine setup_

!> Add a node to the linked list
subroutine add_node_(self, vname, ptr)
  implicit none

  class (ioda_obs_variables) :: self
  character(len=*) :: vname
  type (ioda_obs_var), pointer :: ptr

  type (ioda_obs_var), pointer :: new_node

  ! Create a new node and insert at beginning of list
  allocate(new_node)
  new_node%nobs = 0
  new_node%vname = trim(vname)
  new_node%next => self%head
  self%head => new_node

  ! Return a pointer to the newly inserted node
  ptr => new_node

  ! Keep count of items in the list
  self%n_nodes = self%n_nodes + 1
end subroutine add_node_

!> Find a node in the linked list by key
subroutine get_node_(self, vname, ptr)
  implicit none

  class (ioda_obs_variables) :: self
  character(len=*) :: vname
  type (ioda_obs_var), pointer :: ptr

  type (ioda_obs_var), pointer :: current

  ! Walk the list and look for matching vname
  ptr => NULL()
  current => self%head
  do while (associated(current))
    if (trim(vname) .eq. trim(current%vname)) then
      ptr => current
      exit
    endif

    ! Move to the next node and check
    current => current%next
  enddo
end subroutine get_node_

!> Remove an element from the linked list
subroutine remove_node_(self, vname)
  implicit none

  class (ioda_obs_variables) :: self
  character(len=IODA_OBSVAR_MAXSTRLEN) :: vname

  type (ioda_obs_var), pointer :: prev
  type (ioda_obs_var), pointer :: current

  ! Walk list to find the element matching vname
  current => self%head
  prev => NULL()
  do while (associated(current))
    if (trim(vname) .eq. trim(current%vname)) exit
    prev => current
    current => current%next
  enddo

  ! If found, rewire the list to skip over the node
  if (associated(current)) then
    if (associated(current%next)) then
      ! current is not at the end of the list, so we need to
      ! move the next pointer of the node pointed to by prev
      ! to the next pointer of the node pointed to by current
      if (associated(prev)) then
        ! We are beyond the head of the list
        prev%next => current%next
      else
        ! We are at the head of the list
        self%head%next => current%next
      endif
    endif

    ! Get rid of the node and decrement the list count.
    if (allocated(current%vals)) then
      deallocate(current%vals)
    endif
    deallocate(current)

    self%n_nodes = self%n_nodes - 1
  endif
end subroutine remove_node_

!> Finalize the linked list, deallocate all nodes
subroutine delete_(self)
  implicit none

  class (ioda_obs_variables) :: self

  type (ioda_obs_var), pointer :: current
  type (ioda_obs_var), pointer :: next

  ! Walk the list and deallocate nodes
  current => self%head
  do while (associated(current))
    ! Grab the pointer to the next node
    next => current%next

    ! Delete the current node
    if (allocated(current%vals)) then
      deallocate(current%vals)
    endif
    deallocate(current)
    self%n_nodes = self%n_nodes - 1

    ! Move to the next node
    current => next
  enddo

  self%head => NULL()
end subroutine delete_

end module ioda_obsvar_mod
