C    @(#)sen_jac2.f	20.2 6/27/97
      subroutine sen_jac2 (kt,ksw)
 
C      This subroutine computes Jacobian elements for bus
C      constraints. KSW determines the type.
 
C        0 - Normal Jacobian row for node
C        1 - Jacobian for Q-contraint only, regardless of bus type
 
      include 'ipfinc/parametr.inc'

      include 'ipfinc/alpha.inc'
      include 'ipfinc/alpha2.inc'
      include 'ipfinc/amtrx.inc'
      include 'ipfinc/area.inc'
      include 'ipfinc/beta2.inc'
      include 'ipfinc/blank.inc'
      include 'ipfinc/bus.inc'
      include 'ipfinc/dc.inc'
      include 'ipfinc/ecvar.inc'
      include 'ipfinc/factor.inc'
      include 'ipfinc/gamma.inc'
      include 'ipfinc/lfiles.inc'
      include 'ipfinc/optim1.inc'
      include 'ipfinc/pctvr2.inc'
      include 'ipfinc/slnopt.inc'
      include 'ipfinc/tran.inc'
      include 'ipfinc/xtran.inc'
C
      real ikr,iki,imr,imi
      integer senchk
 
      lp=0
      kl=0
      kp = 0
      isw=0 
      i1 = iflag(kt)
      i2 = iflag(kt+1) - 1  
      if (ksw .eq. 0 .and. kspare(20) .eq. 1) then  

C        Check for slack bus (conditional upon AI_CONTROL option   
C        on > SENSITIVITY < not enabled).  

         do 90 i = i1,i2
         if (jflag(1,i) .eq. 3) then  
           isw = jflag(2,i) 
           if (senchk(kt) .eq. 0) then
              go to 100 
           else 

C             Area slack constraint only  

              call nrpqv (kt,pk,dp,qk,dq,vk)
              lp=1  
              kl = lp   
              korder(0)=1   
              kolum(0)=0
              korder(1)=0   
              kolum(kl) = kt   
              rowh(kl) = 0.0
              rown(kl) = 0.0
              rowj(kl) = 0.0
              rowl(kl) = 1.0
              max = kt 
              mend=2
              go to 294 
           endif
         endif  
   90 continue  
      endif 
  100 ek=e(kt)  
      fk=f(kt)  
      vksq=ek*ek+fk*fk  
      ikr=0.0   
      iki=0.0   

C     Process branches 

      kp = 0                                        
      do l=km(kt), km(kt)-1+kmlen(kt)     
         mt=ikmu(l)                                   
         lp=lp+1   
         if (kp .eq. 0)   then                         
           if (mt .ge. kt)   then                      
              kp = l                                   
              kl = lp                                  
              kolum(lp) = kt                    
              korder(lp) = lp +1                       
              lp = lp +1                               
           endif                                       
         endif                                         
         em=e(mt)  
         fm=f(mt)  
         g12 = gkmu(l)                                
         b12 = bkmu(l)                                
         imr=em*g12-fm*b12 
         imi=em*b12+fm*g12 
         ikr=ikr+imr   
         iki=iki+imi   
         kolum(lp)=mt 
         korder(lp)=lp+1   
         rh=imr*fk-imi*ek  
         rn=imr*ek+imi*fk  
         rj=-rn
         rl=rh 
         rowh(lp)=rh   
         rowj(lp)=rj   
         rown(lp)=rn   
         rowl(lp)=rl   
      enddo

C     Process diagonal 

      if (kp.eq.0) then                             
         kl=lp+1
         lp=kl  
         kolum(lp)=kt  
         mt=kt
      endif 

C     Add identity row for passive BM nodes

      if (lp.eq.1) then 
         rowh(kl) = 1.0 
         rown(kl) = 0.0 
         rowj(kl) = 0.0 
         rowl(kl) = 1.0 
      else 
         g12 = gkku(kt)
         b12 = bkku(kt)
         imr = ek*g12 - fk*b12 
         imi = ek*b12 + fk*g12
         ikr = ikr + imr
         iki = iki + imi
 
         dvk=sqrt(vksq) 
         imr=inetr(kt)*dvk                        
         imi=ineti(kt)*dvk                        
         dpk=ikr*ek+iki*fk+imr  
         dqk=ikr*fk-iki*ek-imi  
         pk = dpk   
         qk = dqk   
         g12 = gkku(kt)*vksq                       
         b12 = bkku(kt)*vksq                       
         rh=-dqk-b12-imi
         rn=dpk+g12 
         rj=dpk-g12-imr 
         rl=dqk-b12 
         rowh(kl)=rh
         rown(kl)=rn
         rowj(kl)=rj
         rowl(kl)=rl
      endif
      korder(lp)=0
      max=mt
      mend=lp+1
      ko=lp
      korder(0)=1
      if (ksw.ne.0) return
  294 mt = senchk(kt)

C     Set up contraints for PV node

      if (mt.ne.0) then
          do 300 i = 1,mend-1
            rowj(i) = 0.0
            rowl(i) = 0.0
  300     continue
          if (mt.eq.kt) then

C            PV node

             rowl(kl) = 1.0 
          else  

C            PV node controlling MT 

             ip = 0 
             ko = korder(ip)
  302        if (kolum(ko)-mt) 304,308,306 
  304        ip=ko  
             ko=korder(ko)  
             if (ko.ne.0) go to 302 
             max = mt  
             lp=mend
  306        korder(mend)=korder(ip)
             korder(ip)=mend
             ko=mend
             mend=mend+1
             kolum(ko)=mt  
             rowh(ko)=0.0   
             rowj(ko)=0.0   
             rown(ko)=0.0   
             rowl(ko)=0.0   
  308        rowl(ko)=1.0   
             rowl(kl) = 0.0001  
          endif 
          if (isw.eq.0) go to 480   
          lp = 1
          ko = 0
          go to 500 
      endif 

C     Implement phase shifter constraints

      do i = i1,i2  
         if (jflag(1,i) .eq. 11 .or. jflag(1,i) .eq. 12) then
            if (jflag(1,i) .eq. 11) then
               ax = 1.0
            else
               ax = -1.0
            endif
            ji = jflag(2,i)
            mt = ntot + ntopt + dabs(txtie(8,ji))
            max = mt  
            korder(lp) = mend
            lp = mend
            mend = mend+1
            kolum(lp) = mt  
            rowh(lp) = ax
            rowj(lp) = 0.0   
            rown(lp) = 0.0   
            rowl(lp) = ax
            korder(lp) = 0
            go to 320
         endif
      enddo
  320 continue

  480 if (isw .eq. 0) go to 780
C
C     Nullify P constraint in Jacobian. It will be replaced with
C     an Area Interchange constraint.
C
      do i = 1,mend-1
         rowh(i) = 0.0
         rown(i) = 0.0
      enddo
      lp=korder(0)
      ko=0

C     Define area interchange constraints

  500 xatot=0.0
      jt=isw
      if(jt.eq.0) call erexit   
      j1=karea(3,jt)
      js=karea(4,jt)+j1-1   
      do 700 j=j1,js
      ix=kaloc(j)   
      iax=iabs(ix)  
      k1=tie(1,iax)
      k2=tie(7,iax)
      mt=k1 
      if (ix.lt.0) mt=k2
      ka1=tie(2,iax)   
      kdc=tie(9,iax)   
      if (kdc.eq.0) go to 580   
      if (ix.lt.0) go to 700
      kd = kdc  
  530 k1x = dmin1 (dcline(1,kd),dcline(2,kd))   
      k2x = dmax1 (dcline(1,kd),dcline(2,kd))   
      if (k1x.ne.min0(k1,k2)) then  
         if (kd.ne.kdc) call erexit 
         if (mtdcln.eq.0) call erexit   
         kd = kdc + mtdcln  
         go to 530  
      else if (k2x.ne.max0(k1,k2)) then 
         call erexit
      endif 
      if (k1.eq.dcline(1,kd)) then  
         l1 = dcline(8,kd)  
         l2 = dcline(9,kd)  
      else  
         l1 = dcline(9,kd)  
         l2 = dcline(8,kd)  
      endif 
      v1=dcbus(20,l1)   
      v2=dcbus(20,l2)   
      pin = v1*(v1 - v2)/(dcline(4,kd)*bmva)
      if (jt.ne.ka1) pin = -pin 
      xatot = xatot + pin   
      if (idswb.ne.0) write (dbug,570)kt,pin
  570 format(' DC export from area slack node ',i4,' is ',f7.3)
      go to 700

  580 continue
  590 if (kolum(lp)-mt) 600,620,610
  620 ko=lp
      go to 630

  600 ko=lp
      lp=korder(lp)
      if (lp.ne.0) go to 590
      max=mt   
  610 korder(mend)=korder(ko)   
      korder(ko)=mend   
      kolum(mend)=mt   
      ko=mend   
      lp=mend   
      mend=mend+1   
      rowh(lp)=0.0  
      rown(lp)=0.0  
      rowj(lp)=0.0  
      rowl(lp)=0.0  
  630 ek=e(k1)  
      fk=f(k1)  
      em=e(k2)  
      fm=f(k2)  
      vksq=ek*ek+fk*fk  
      g12=tie(5,iax)
      b12=tie(6,iax)
      ikr=g12*em-b12*fm 
      iki=b12*em+g12*fm 
      rh=-ek*iki+fk*ikr 
      rn=ek*ikr+fk*iki  
      pin=rn+vksq*tie(3,iax)
      if (ix.gt.0) then 
         rh=-rh 
         rn=rn+2.0*vksq*tie(3,iax)  
      endif 
      if (ka1.ne.jt) then   
         pin=-pin   
         rh=-rh 
         rn=-rn 
      endif 
      rowh(lp)=rowh(lp)+rh  
      rown(lp)=rown(lp)+rn  
  690 if (ix.gt.0) xatot=xatot+pin  
  700 continue  
  710 if (kolum(lp)-max) 720,740,730
  720 lp=korder(lp)
      go to 710

  730 call erexit

  740 continue
C
C     The following logic projects a target PNET for
c     the area slack bus such that the ensuing convergence
c     test in SWITCH is applicable.
C
      if (idswc.gt.0) then
         write (dbug,760) jt,kt,area(2,jt),xatot
  760    format ('0 Area interchange ',2i6,4e20.6)  
         l = 0  
         write (dbug,770) l,kolum(l),korder(l)  
  770    format ('  Debug of area Jacobian '/(3i8,4e20.6))  
         ix=mend-1  
         write (dbug,772) (l,kolum(l),korder(l),rowh(l),rown(l),rowj(l),
     1    rowl(l),l=1,ix)   
  772    format (3i8,4e20.6)
      endif 
  780 continue  
      return
      end   
