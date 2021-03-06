C    %W% %G%
      subroutine cmde2_out (scrfil, filename)
      integer scrfil
      character filename *(*)
 
C     THIS ROUTINE COMPILES THE RESULTS OF ALL COMMON MODE CHANGES
 
      include 'ipfinc/parametr.inc'

      include 'ipfinc/alpha.inc'
      include 'ipfinc/blank.inc'
      include 'ipfinc/branch.inc'
      include 'ipfinc/bus.inc'
      include 'ipfinc/cont.inc'
      include 'ipfinc/intbus.inc'
      include 'ipfinc/cmde_com.inc'
      include 'ipfinc/outxrf.inc'
      include 'ipfinc/prt.inc'
      include 'ipfinc/comm_mode.inc'
      include 'ipfinc/time1.inc'
      include 'ipfinc/brtype.inc'
 
      dimension pctime(10)  
      external kp_outcmd2, sp_outcmd2, kmp_cmde2, swp_cmde2, 
     &         kp_vltcmd2, sp_vltcmd2, kpx_cmde2, spx_cmde2   
      integer ptr
      character id*1, jd*1, cbown*3, brown*3, tag*3, rattag*1, 
     &          rec_type(4)*1, chg_type(4)*1, datein*5, 
     &          sol_prob_text*11, br_own*3
 
      logical finished
 
      data rec_type / 'B', '+', 'L', 'T' /
      data chg_type / 'D', 'M', ' ', 'R' /
C       
C     Initialize arrays. 
C       
      do i = 1, novl
        ibrol(i) = 0
      enddo
C
C          PREPARE OUTAGE - OVERLOAD ANALYSIS
C
C          KASE1(3) - ANALYSIS LISTING SELECTION
C                0  = OUTAGE-OVERLOAD ONLY  
C                1  = OVERLOAD-OUTAGE ONLY  
C                2  = BOTH  
C       
C          KASE1(5) - ANALYSIS LISTING SORT ORDER   
C                0  = BUS   
C                1  = OWNERSHIP-BUS 
        
      if (kase1(3) .ne. 1) then 
         write (outbuf,120) 
  120    format(30x,'Summary of bus and line problems for each outage') 
         write (scrfil, fmt='(a)'), outbuf
         write (outbuf,130) 
  130    format(5x,'* * O U T A G E * *')   
         write (scrfil, fmt='(a)'), outbuf
      endif 
C       
      iic = 0   
      iid = 0   
      iie = 0   
C       
C     Determine sort order   
C       
      do i = 1, nout+num_comm_mode
         isort(i) = i  
      enddo
        
      if (kase1(5) .eq. 1) then 
         call qiksrt (1, nout+num_comm_mode, kp_outcmd2, sp_outcmd2) 
      endif 
        
      do 290 is = 1, nout+num_comm_mode
         icntg = isort(is)  
         if (ipbad(icntg) .eq. ipbad(icntg+1)) go to 290
         i1 = 0 
c
c        "i2" is pass switch: -1 -> process overloads
c                              0 -> process voltage problems
c
         i2 = -1
         i3 = 0     ! count of overloads for this outage
         i4 = 0
         do j = ipbad(icntg), ipbad(icntg+1) - 1
            if ( ibad(j) .ne. 0 ) i4 = i4 + 1
         enddo
         write (outbuf, 152) comm_mode(icntg)(1:38)
  152    format('0 ', a, 13x, 24(' -'))   
         write (scrfil, fmt='(a)') outbuf
         outbuf = ' '
         write (scrfil, fmt='(a)') outbuf
         call lis_cmde2( icntg, 1, scrfil )

  160    do 280 j = ipbad(icntg), ipbad(icntg+1) - 1
            if (ibad(j)) 170, 250, 210  
C       
C           BRANCH OVERLOAD
C       
  170       if (i2 .ge. 0) go to 280
            k = -ibad(j)
            k1 = klno(1,k)  
            k2 = klno(2,k)  
            call getchr(1,jd,klno(5,k)) 
            call getchr(3,brown,klno(3,k))  
            if (clno(11,k) .gt. 0.0) then   
               tag = 'AMP'  
            else
               tag = 'MVA'  
            endif   
        
            rat = abs(clno(12,k))   
            bad(j) = sqrt (bad(j)) * abs(clno(11,k)/rat)
            ratact = bad(j) * rat   
            baslod = clnobase(k) * rat        
            ptr = outxrf(k) 
            call getrat (ptr, rating, rattag, ratnom, ratthr,   
     &                   ratllf, ratbtl)
            nbr = iabs(brnch_ptr(ptr))        
            kdin = kbrnch(11,nbr)
            if (brtype(ptr) .eq. BRTYP_PEQ) kdin=0
            kyr = kdin/100
            mon = mod(kdin,100) 
            if (kyr .eq. 0) then
               datein = ' '
            else
               write (datein, 172) mon, kyr
  172          format (i2.2, '/', i2.2)

            endif
        
            if (kase1(3) .ne. 1) then   
               if (i1 .eq. 0) then  
                  call chkbtm(5)
                  write (outbuf,180)
  180             format('0',t26,'Overloads',t76,'Rating',t86,'Type',   
     &               t94,'per unit / pst-cont  pre-cont Date in')   
                  write (scrfil, fmt='(a)') outbuf
               endif
               write (outbuf,190) bus(k1), base(k1), bus(k2),   
     &            base(k2), jd, brsect(ptr), brown,
     &            zone(k1), zone(k2), rat, tag,  
     &            rattag, bad(j), ratact, baslod, datein
  190          format(t28,a8,f6.1,2x,a8,f6.1,1x,a1,1x,i1,2x,a3,
     &            1x, 2(1x,a2),f6.0, 
     &            1x,a3,t88,a1,t96,f6.3,' /',f7.1, f10.1, 3x, a)  
               write (scrfil, fmt='(a)') outbuf   
               i3 = i3 + 1
            endif   
            i1 = +1 
C       
C           LINK BRANCH OVERLOAD TO OUTAGE 
C       
            if (iic .lt. MXIBAD) then 
               iic = iic + 1
            else if (iic .eq. MXIBAD) then
               iic = iic + 1
               write (errbuf(1), 193)  comm_mode(icntg-nout)(1:31), 
     &            bus(k1), base(k1), bus(k2), base(k2), jd, iic   
  193          format (' Overflow by contingency ', a, 
     &                 ' Overload ', a8, f6.1, 1x, a8, f6.1, 1x, a1, 
     &                 ' IIC ', i6)  
               call prterx ('E',1)  
            else
               iic = iic + 1
               go to 280
            endif   
            if (ibrol(k) .eq. 0) then   
               ibrol(k) = iic   
               else 
               jjc = ibrol(k)   
               do while (ibrolp(2,jjc) .ne. 0) 
                  jjcx = ibrolp(2,jjc)
                  jjc = jjcx
               enddo
               ibrolp(2,jjc) = iic
            endif   
            ibrolp(1,iic) = icntg
            ibrolp(2,iic) = 0
            brolp(iic) = bad(j) 
            go to 280   
C       
C           VOLTAGE PROBLEMS   
C       
  210       if (i2 .eq. -1) go to 280   
            k = ibad(j) 
            kt = inp2opt(k)
            vactual = bad(j) * base(k)
            vchange = bad(j) - dsqrt(e(kt)**2 + f(kt)**2)        
            if (kase1(3) .ne. 1) then   
               if (i2 .eq. 0) then  
                  call chkbtm(5)
                  write (outbuf,220)
  220             format('0',t26,'Bus voltage violation ',t76,'Actual', 
     &               t86,'Per unit  Actual / change /(    limits   )') 
                  write (scrfil, fmt='(a)') outbuf
               endif
               write (outbuf,230) bus(k), base(k), owner(k), 
     &            zone(k), vactual, bad(j), vchange, vlimn(kt), 
     &            vlimx(kt)
  230          format(t28,a8,f6.1,2x,a3,2x,a2,
     &            t74,f6.0,' KV',t96,f6.3,' /', f7.3,' /(',f6.3,',',
     &            f6.3,')' ) 
               write (scrfil, fmt='(a)') outbuf   
            endif   
            i2 = i2 + 1 
C       
C           Link bus over/undervoltage 
C       
            if (iid .lt. MXIBAD) then 
               iid = iid + 1
            else if (iid .eq. MXIBAD) then
               iid = iid + 1
               write (errbuf(1), 233)  comm_mode(icntg-nout)(1:31),
     &           bus(k1), base(k1), iid   
  233          format (' Overflow by contingency ', a, 
     &            ' Under/overvoltage ', a8, f6.1, ' IID ', i6)  
               call prterx ('E',1)  
            else
               iid = iid + 1
               go to 280
            endif   
            if (ibsol(k) .eq. 0) then   
               ibsol(k) = iid   
            else
               jjc = ibsol(k)   
               do while (ibsolp(2,jjc) .ne. 0) 
                  jjcx = ibsolp(2,jjc)
                  jjc = jjcx
               enddo
               ibsolp(2,jjc) = iid
            endif   
            ibsolp(1,iid) = icntg
            ibsolp(2,iid) = 0
            bsolp(iid) = bad(j) 
            go to 280   
c
c           System problems
c
  250       if (i2 .eq. -1) go to 280
            if (bad(j) .gt. 0.0) go to 270  
C       
C           Examine system separation for isolated generator  
C       
            if (bad(j) .lt. 0) then
               k = -bad(j)
               if (iid .lt. MXIBAD) then 
                  iid = iid + 1
               else if (iid .eq. MXIBAD) then
                  iid = iid + 1
                  write (errbuf(1), 264)  comm_mode(icntg-nout)(1:31), 
     &               bus(k), base(k), iid 
  264             format (' Overflow by contingency ', a, 
     &               ' Isolated bus ', a8, f6.1, ' IID ', i6) 
                  call prterx ('E',1)  
               else
                  iid = iid + 1
                  go to 280
               endif   
            else
               go to 270
            endif
            if (ibsol(k) .eq. 0) then   
               ibsol(k) = iid   
            else
               jjc = ibsol(k)   
               do while (ibsolp(2,jjc) .ne. 0) 
                  jjcx = ibsolp(2,jjc)
                  jjc = jjcx
               enddo
               ibsolp(2,jjc) = iid
            endif   
            ibsolp(1,iid) = icntg
            ibsolp(2,iid) = 0
            bsolp(iid) = 0.0
            goto 280
  270       if (iie .lt. MXCNBR/2) then 
               iie = iie + 1
            else if (iie .eq. MXCNBR/2) then
               iie = iie + 1
               write (errbuf(1), 274)  comm_mode(icntg-nout)(1:31), 
     &           iie   
  274          format (' Overflow by contingency ', a, 
     &            ' Separation IIE ', i6) 
               call prterx ('E',1)  
            else
               iie = iie + 1
               go to 280
            endif   
            brcon(iie) = bad(j) 
            ibrcon(iie) = icntg
            if ( ibad_rs(icntg) .eq. 1 .and. i3 .ne. 0 ) then
               write (outbuf,278)
  278          format('0',t26,'Failed reactive solution')
               write (scrfil, fmt='(a)') outbuf
            endif
  280    continue   
         if (i2 .eq. -1) then   
            i2 = 0  
            go to 160   
         endif  
  290 continue  
C       
C     List non-convergence and system separation 
C       
      if (iie .eq. 0) goto 380  
      write (outbuf,300)
  300 format(20x,'Summary of System Separations and Divergences')   
      write (scrfil, fmt='(a)'), outbuf
      write (outbuf,310)
  310 format('0--------   Outage  ',13('-'),
     &       ' ID  Owner  Zones  -------  Problem')
      write (scrfil, fmt='(a)'), outbuf
      outbuf = ' '  
      write (scrfil, fmt='(a)'), outbuf
        
      lim = min0 (iie,MXCNBR/2) 
      do i = 1, lim 
         j = ibrcon(i)  
         l = brcon(i)   
         if ( ( j .eq. ibrcon(i+1) .and. ibad_rs(j) .eq. 2 ) .or.
     &        ibad_rs(j) .eq. 1 ) then
            sol_prob_text = ' (Reactive)'
         else
            sol_prob_text = ' '
         endif
C       
C           Test for system separation 
C       
         if (mod(l,10) .eq. 0) then 
            write (outbuf,322) comm_mode(j-nout)(1:40),
     &         sol_prob_text 
  322       format(2x, a, 17x, 'Causes system separation', a )  
            write (scrfil, fmt='(a)') outbuf  
            call lis_cmde2( j-nout, 2, scrfil )
C       
C           Test for iteration limit   
C       
         else if(mod(l,10) .eq. 1) then 
            l10 = l / 10
            write (outbuf,332) comm_mode(j-nout)(1:40), l10,
     &        sol_prob_text 
  332       format(2x, a, 17x, 
     &         'No solution  - (',i3,' iterations)', a )
            write (scrfil, fmt='(a)') outbuf  
            call lis_cmde2( j-nout, 2, scrfil )
C       
C           Test for maximum angle excursion   
C       
         else if(mod(l,10) .eq. 2) then 
            ix = l / 10 
            x = 0.001 * ix  
            write (outbuf,342) comm_mode(j-nout)(1:40), x,
     &         sol_prob_text 
  342       format(2x, a, 17x, 
     &        'No solution - insufficient capacity (max angle ',
     &         e10.3,')', a )   
            write (scrfil, fmt='(a)') outbuf  
            call lis_cmde2( j-nout, 2, scrfil )
        
         else if (mod(l,10) .eq. 3) then
            x = l / 10  
            x = 0.001 * x   
            write (outbuf,352) comm_mode(j-nout)(1:40), x,
     &         sol_prob_text 
  352       format(2x, a, 17x, 
     &         'No solution - insufficient reactive (min volt', 
     &         e10.3,')', a ) 
            write (scrfil, fmt='(a)') outbuf  
            call lis_cmde2( j-nout, 2, scrfil )
C       
C           Test for arithmetic trap, floating overflow
C       
         else   
            write (outbuf,362) comm_mode(j-nout)(1:40),
     &         sol_prob_text 
  362       format(2x, a, 17x, 
     &         'Causes arithmetic trap, floating overflow', a ) 
            write (scrfil, fmt='(a)') outbuf  
            call lis_cmde2( j-nout, 2, scrfil )
         endif
        
      enddo
  380 continue  
C       
C     List overloads and their problems  
C       
      if (iic .eq. 0 .or. kase1(3) .eq. 0) go to 470
      call forbtm   
      write (outbuf,390)
  390 format(40x,'Summary of overloaded lines caused by outages')   
      write (scrfil, fmt='(a)'), outbuf
      write (outbuf,400)
  400 format('0',6x,' * O V E R L O A D  *')
      write (scrfil, fmt='(a)'), outbuf
      outbuf = ' '  
      write (scrfil, fmt='(a)'), outbuf
C       
C     Determine sort order   
C       
      do 410 i = 1, novl
  410 isort(i) = i  
        
      if (kase1(5) .eq. 1) then 
         call qiksrt (1, novl, kmp_cmde2, swp_cmde2) 
      endif 
        
      do 460 is = 1, novl   
         i = isort(is)  
         if (ibrol(i) .eq. 0) then  
            if (abs(clno(11,i)) / (tpc * abs(clno(12,i))) .le. 1.001)   
     &         goto 460 
         endif  
        
         k1 = klno(1,i) 
         k2 = klno(2,i) 
         call getchr(1,id,klno(5,i))
         if (clno(11,i) .gt. 0.0) then  
            tag = 'AMP' 
         else   
            tag = 'MVA' 
         endif  
         call chkbtm(7) 
         ptr = outxrf(i)
         nbr = iabs (brnch_ptr(ptr))
         call getrat (ptr, rat, rattag, ratnom, ratthr, ratllf, 
     &                ratbtl)   
         irat = ratnom + 0.5
         kdin = kbrnch(11,nbr)
         if (brtype(ptr) .eq. BRTYP_PEQ) kdin=0
         kyr = kdin/100
         mon = mod(kdin,100) 
         if (kyr .eq. 0) then
            datein = ' '
         else
            write (datein, 172) mon, kyr
         endif
         if ((rateln(1,nbr)+rateln(2,nbr)+rateln(3,nbr)) .eq. 0) then   
            write (outbuf,420) bus(k1), base(k1), bus(k2),  
     &         base(k2), id, brsect(ptr), klno(3,i),
     &         zone(k1),zone(k2), irat, tag, datein
  420       format('0',2(a8,f6.1,2x),a1,2x,i1,2x,a3,1x,2(1x,a2), 
     &         '  Overload % of ratings (Nominal:', i6, 1x, a3, ',', 
     &         t118, 'IN ', a)
            write (scrfil, fmt='(a)') outbuf  
            if (clno(11,i) .gt. 0.0) then   
               write (outbuf,424) 0, tag, 0, tag   
               write (scrfil, fmt='(a)') outbuf   
            else
               write (outbuf,428) 0, tag, 0, tag, 0, tag  
               write (scrfil, fmt='(a)') outbuf   
            endif   
         else   
            irate1 = rateln(1,nbr) + 0.5
            irate2 = rateln(2,nbr) + 0.5
            irate3 = rateln(3,nbr) + 0.5
            if (clno(11,i) .gt. 0.0) then   
               write (outbuf,422) bus(k1), base(k1), bus(k2),
     &            base(k2), id, brsect(ptr), klno(3,i),
     &            zone(k1), zone(k2), irat, tag, 
     &            datein
  422          format('0',2(a8,f6.1,2x),a1,2x,i1,2x,a3,1x,2(1x,a2),
     &            '  Overload % of ratings (Nominal:',i6,1x, a3, ',',
     &            t118, 'IN ', a)
               write (scrfil, fmt='(a)') outbuf   
               write (outbuf,424) irate1, tag, irate2, tag   
  424          format(t35, 'Thermal:',i6, 1x, a3, ', Bottleneck:', i6,
     &            1x, a3, 1x, ') from outages:')
               write (scrfil, fmt='(a)') outbuf   
            else
               write (outbuf,426) bus(k1), base(k1), bus(k2),   
     &            base(k2), id, brsect(ptr), klno(3,i),
     &            zone(k1), zone(k2), irat, tag, 
     &            datein
  426          format('0',2(a8,f6.1,2x),a1,2x,i1,2x,a3,1x,2(1x,a2),
     &            '  Overload % of ratings (Nominal:',i6,1x,a3, ',',
     &            t118, 'IN ', a)
               write (scrfil, fmt='(a)') outbuf   
               write (outbuf,428) irate1, tag, irate3, tag, irate2, 
     &            tag  
  428          format(t35, 'Thermal:',i6,1x,a3,', Bottleneck:',i6,1x,a3,
     &            ',  Loss of Life:',i6,1x,a3,1x,') from outages:')  
               write (scrfil, fmt='(a)') outbuf   
            endif   
         endif  

         ixovl = 0  
         j = ibrol(i)   
         do while (j .gt. 0) 
            ixovl = ixovl + 1   
            xsort(ixovl) = j
            jx = ibrolp(2,j)
            j = jx
         enddo

         if ((clnobase(i) / tpc .gt. 1.001) .or. (ixovl .gt. 0)) then 
            pcrat = clnobase(i) * 100.0
            ovrld = rat * pcrat / 100.0 
            write (outbuf,430) pcrat, ovrld, rattag   
  430       format(t49, f7.1, t61, '(', f8.1, ' )', t74, a1, t83,   
     &         '** Basecase **') 
            write (scrfil, fmt='(a)') outbuf  
         endif

         if (ixovl .gt. 0) then 
            call qiksrt (1, ixovl, kpx_cmde2, spx_cmde2)  
            do js = 1, ixovl
               j = xsort(js)   
               k = ibrolp(1,j)
               pcrat = brolp(j) * 100.0 
               ovrld = rat * pcrat / 100. 
               write (outbuf,452) pcrat, ovrld, rattag, 
     &            comm_mode(k)(1:31)
  452          format(t49, f7.1, t61, '(', f8.1, ' )', t74, a1, t83, a)
               write (scrfil, fmt='(a)') outbuf  
            enddo
         endif  
  460 continue  
  470 continue  
C       
C     List under/overvoltages and bus isolations by outage   
C       
      if (iid .eq. 0 .or. kase1(3) .eq. 0) go to 580
      call forbtm   
      write (outbuf,480)
  480 format(30x,'Summary of Bus Over/Under Voltages and Bus Separations
     1')
      write (scrfil, fmt='(a)'), outbuf
      write (outbuf,490)
  490 format(t1,'      Bus       Owner       Problem',
     &      t46,'Caused by outage of:', 
     &      t102,'MAG/PER UNIT') 
      write (scrfil, fmt='(a)'), outbuf
      write (outbuf,500)
  500 format('      ---       -----       -------', 10x,38('-'),18x,
     &  '--------------')   
      write (scrfil, fmt='(a)'), outbuf
        
      ixovl = 0 
      do k = 1, ntot
         i = opt2inp(k)   
         if (ibsol(i) .ne. 0) then
           ixovl = ixovl + 1  
           isort(ixovl) = i   
         endif
      enddo
        
      if (kase1(5) .eq. 1) then 
         call qiksrt (1, ixovl, kp_vltcmd2, sp_vltcmd2)
      endif 
        
      do 570 is = 1, ixovl  
         i = isort(is)  
         if (ibsol(i) .eq. 0) go to 570 
         l = ibsol(i)   
         iprt = 0   
         finished = .false.
         do while (.not. finished)
            j = ibsolp(1,l)   
            if (bsolp(l) .eq. 0.0) then
               if (iprt .eq. 0) then   
                  write (outbuf,542) bus(i), base(i),
     &               owner(i), zone(i),
     &               comm_mode(j)(1:40)
  542             format('0 ', a8, f6.1, 2x, a3, 2x, a2, 3x,
     &                  'Separates', 8x, a)
               else
                  write (outbuf,544) comm_mode(j)(1:40)
  544             format(28x, 'Separates', 8x, a)
               endif   
               write (scrfil, fmt='(a)') outbuf  
            else   
               rat = bsolp(l) * base(i)  
               if (iprt .eq. 0) then   
                  write (outbuf, 562) bus(i), base(i), 
     &               owner(i), zone(i),
     &               comm_mode(j)(1:40), rat, bsolp(l)  
  562             format('0 ', a8, f6.1, 2x, a3, 2x, a2, 3x,
     &                  'Voltage', 10x, a, 15x, f6.1, ' / ', f6.3)   
               else
                  write (outbuf, 564) comm_mode(j)(1:40), rat, 
     &               bsolp(l)   
  564             format(28x, 'Voltage', 10x, a, 15x, f6.1, ' / ', 
     &               f6.3)  
               endif   
               write (scrfil, fmt='(a)') outbuf  
            endif  
            iprt = 1   
            if (ibsolp(2,l) .gt. 0) then   
               l = ibsolp(2,l)
            else
               finished = .true.
            endif
         enddo
  570 continue  
  580 continue  
      outbuf = ' '  
      write (scrfil, fmt='(a)'), outbuf
        
C     PROGRAM STATISTICS REPORT   

      do i = 1, 10  
        pctime(i) = 0.0
      enddo
        
      call space(2) 
      write (outbuf,600)
  600 format(' TIMING STATISTICS')  
      write (scrfil, fmt='(a)') outbuf
      call space (2)
      write (outbuf,610)
  610 format(7x,'TIME (MSEC)       PERCENT')
      write (scrfil, fmt='(a)') outbuf
      write (outbuf,680) time(7), pctime(7) 
  680 format(4x,2f13.2,5x,'TIME FOR CHECKING OVERLOADS  (CHECK)')   
      write (scrfil, fmt='(a)') outbuf
      return

      end   
