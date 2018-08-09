module gnssro_mod_obserr
!==========================================================================
use kinds

contains

subroutine  refractivity_err_gsi(obsLat, obsZ, nobs, GlobalModel, obsErr)
integer,         intent(in)                   :: nobs
real(kind_real), dimension(nobs), intent(in)  :: obsLat,  obsZ
real(kind_real), dimension(nobs), intent(out) :: obsErr
logical,         intent(in)                   :: GlobalModel
real(kind_real), dimension(nobs)              :: obsZ_km

obsZ_km  = obsZ / 1000.0_kind_real

if( GlobalModel ) then ! for global
 
  do i = 1, nobs     
     if( obsLat(i)>= 20.0 .or.obsLat(i)<= -20.0 ) then
         obsErr(i)=-1.321_kind_real+0.341_kind_real*obsZ_km(i)-0.005_kind_real*obsZ_km(i)**2
     else
       if(obsZ_km(i) > 10.0) then
          obsErr(i)=2.013_kind_real-0.060_kind_real*obsZ_km(i)+0.0045_kind_real*obsZ_km(i)**2
       else
          obsErr(i)=-1.18_kind_real+0.058_kind_real*obsZ_km(i)+0.025_kind_real*obsZ_km(i)**2
       endif 
     endif
     obsErr(i) = 1.0_kind_real/abs(exp(obsErr(i)))
  end do

else ! for regional 
  do i = 1, nobs
     if( obsLat(i) >= 20.0 .or.obsLat(i) <= -20.0 ) then
         if (obsZ_km(i) > 10.00) then
             obsErr(i) =-1.321_kind_real+0.341_kind_real*obsZ_km(i)-0.005_kind_real*obsZ_km(i)**2
         else
             obsErr(i) =-1.2_kind_real+0.065_kind_real*obsZ_km(i)+0.021_kind_real*obsZ_km(i)**2
         endif
     else
         if(obsZ_km(i) > 10.00) then
            obsErr(i) =2.013_kind_real-0.120_kind_real*obsZ_km(i)+0.0065_kind_real*obsZ_km(i)**2
         else
            obsErr(i) =-1.19_kind_real+0.03_kind_real*obsZ_km(i)+0.023_kind_real*obsZ_km(i)**2
         endif
     endif
     obsErr(i) = 1.0_kind_real/abs(exp(obsErr(i)))
 end do

endif

end subroutine refractivity_err_gsi

end module gnssro_mod_obserr
