!
! (C) Copyright 2022 UCAR
!
! This software is licensed under the terms of the Apache Licence Version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

!>  Test the ioda Fortran vector<string> interface

program ioda_fortran_vecstring_test
use,intrinsic :: iso_c_binding
use,intrinsic :: iso_fortran_env
use :: ioda_vecstring_mod
use :: ioda_f_c_string_mod
    
   implicit none  
   type(ioda_vecstring) :: vstr,vstr2
   type(ioda_string) :: str1,str2
   integer(int64) :: n,i,sz,exp_sz,m
   character(len=256) :: fstr1
   character(len=256) :: fstr3
   character(len=256) :: fstr2
   character(len=256) :: numstr
   character(len=256) :: astr
   character(len=256) :: fstr
   type(c_ptr) :: ptr

   fstr1 = 'this a test string'
      
   call ioda_string_init(str1)
   call str1%set(fstr1)
   call str1%get(fstr2)
   
   write(*,*)' input = ',trim(fstr1)
   write(*,*)' result = ',trim(fstr2)     
   
   do i=1,len_trim(fstr1)
      if ( fstr1(i:i) .ne. fstr2(i:i) ) then
          write(error_unit,*)'ioda_string get/set failed'
          write(error_unit,*)'set ',trim(fstr1)
          write(error_unit,*)'get ',trim(fstr2)
          stop -1
      end if
   end do
   
   m = 4
   
   write(astr,'(a,i1)')' ',m
   
   write(numstr,'(a,a)')trim(fstr1),trim(astr)
   
   call str1%append(astr)

   call str1%get(fstr2) 
   
   write(*,*)' append input  = ',trim(astr),'_'
   write(*,*)' append result = ',trim(fstr2),'_'     
   write(*,*)' expected      = ',trim(numstr),'_'
   do i=1,len_trim(fstr2)
      if ( numstr(i:i) .ne. fstr2(i:i) ) then
          write(error_unit,*)'ioda_string get/append failed'
          write(error_unit,*)'app ',trim(numstr),'_'
          write(error_unit,*)'get ',trim(fstr2),'_'
          write(error_unit,*)'app ',numstr(i:i)
          write(error_unit,*)'get ',fstr2(i:i)
          stop -1
      end if
   end do
   
   sz = str1%size()
   
   exp_sz = len_trim(fstr2)
   
   if ( sz .ne. exp_sz) then
      write(error_unit,*)' ioda string size failed'
      write(error_unit,*)' expected ',exp_sz,' got ',sz
      stop -1
   else
      write(Error_unit,*)' ioda string size worked' 
   end if
   
   call str1%clear()
   sz = str1%size()
   exp_sz = 0
   
   if ( sz .ne. exp_sz) then
      write(error_unit,*)' ioda string clear failed'
      write(error_unit,*)' expected ',exp_sz,' got ',sz
      stop -1
   else
      write(Error_unit,*)' ioda string clear worked' 
   end if

   write(error_unit,*)'TEST OF STRING-STRING'

   fstr1 = 'this a test string' 
   
   call ioda_string_init(str2)
   
   call str1%set(fstr1)

   call str1%get(fstr2)
   
   write(*,*)' input = ',trim(fstr1)
   write(*,*)' result = ',trim(fstr2)     
   
   do i=1,len_trim(fstr1)
      if ( fstr1(i:i) .ne. fstr2(i:i) ) then
          write(error_unit,*)'ioda_string get/set failed'
          write(error_unit,*)'set ',trim(fstr1)
          write(error_unit,*)'get ',trim(fstr2)
          stop -1
      end if
   end do
      
   call str2%set_string(str1)
   
   call str2%get(fstr2)
   
   write(*,*)' input  = ',trim(fstr1),'>'
   write(*,*)' result = ',trim(fstr2),'>'     
   
   do i=1,len_trim(fstr1)
      if ( fstr1(i:i) .ne. fstr2(i:i) ) then
          write(error_unit,*)'ioda_string get/set_string failed'
          write(error_unit,*)'set ',trim(fstr1)
          write(error_unit,*)'get ',trim(fstr2)
          write(error_unit,*)'set ',fstr1(i:i)
          write(error_unit,*)'get ',fstr2(i:i)
          stop -1
      end if
   end do
   
   m = 4
   
   write(astr,'(a,i1)')' ',m
   
   write(numstr,'(a,a)')trim(fstr1),trim(astr)

   call str1%set(astr)
   
   call str2%append_string(str1)

   call str2%get(fstr2) 
   
   write(*,*)' append input  = ',trim(astr),'_'
   write(*,*)' append result = ',trim(fstr2),'_'     
   write(*,*)' expected      = ',trim(numstr),'_'
   do i=1,len_trim(fstr2)
      if ( numstr(i:i) .ne. fstr2(i:i) ) then
          write(error_unit,*)'ioda_string get/append failed'
          write(error_unit,*)'app ',trim(numstr),'_'
          write(error_unit,*)'get ',trim(fstr2),'_'
          write(error_unit,*)'app ',numstr(i:i)
          write(error_unit,*)'get ',fstr2(i:i)
          stop -1
      end if
   end do
   

   write(error_unit,*)'TEST OF VECSTRING'

!
!  test vecstring  
!
   call ioda_vecstring_init(vstr) 

   do i=1,4
      m = i
      write(fstr1,'(a,i1)')'this is a test ',m 
      call vstr%push_back(fstr1)
      if ( i == 2 ) then
      	 exp_sz = len_Trim(fstr1)
      end if
      write(error_unit,'(i2,a,a,a)')i,' push back ',trim(fstr1),'>'
   end do

   do i=1,4
      call vstr%get(i,fstr2) 	
      write(error_unit,'(i2,a,a,a)')i,' got       ',trim(fstr2),'>'
   end do

   sz = vstr%size()
   m  = 4   
   if ( sz .ne. m ) then
      write(error_unit,*)'ioda vecstring size failed '
      write(error_unit,*)' size = 4 but got ',sz
      stop -1	
   end if

   write(error_unit,*)'testing element size'   
   m = 2
   sz = vstr%element_size(m)
   if ( sz .ne. exp_sz ) then
      write(error_unit,*)'ioda vecstring element size failed '
      write(error_unit,*)' size = ',exp_sz,' but got ',sz
      stop -1	
   end if

   write(error_unit,*)'testing copy'
   call ioda_vecstring_init(vstr2)
   vstr2 = vstr
   sz = vstr%size()   
   do i=1,sz
     call vstr%get(i,fstr1)
     call vstr2%get(i,fstr)
     write(error_unit,*)i,' ',fstr1,' ',fstr
     if ( fstr1 /= fstr) then
	stop 'strings are not equal in copied vector string'
     end if
   end do    

   
   stop 0
end program ioda_fortran_vecstring_test
