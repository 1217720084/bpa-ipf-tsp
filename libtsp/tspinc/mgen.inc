*BEGIN MGEN
      COMMON/MGEN/MGEN(16*MAXGEN)
                DIMENSION MANGL(MAXGEN),MANGP(MAXGEN),MVFLD(MAXGEN),
     1          MEPQ(MAXGEN),MFSAT(MAXGEN),MGOV(MAXGEN),MGENP(MAXGEN),
     2          MEXSAT(MAXGEN),MREG(MAXGEN),MPACC(MAXGEN),MGENQ(MAXGEN),
     3          MSUP(MAXGEN),MTQDP(MAXGEN),MFCUR(MAXGEN),MPA(MAXGEN),
     4          MCUR(MAXGEN),MXSUP(MAXGEN),MBUS(MAXBUS)
      EQUIVALENCE (MGEN,MANGL), (MGEN(MAXGEN +1),MANGP),
     1  (MGEN(2*MAXGEN +1),MVFLD), (MGEN(3*MAXGEN +1),MEPQ),
     2  (MGEN(4*MAXGEN +1),MFSAT), (MGEN(5*MAXGEN +1),MGOV),
     3  (MGEN(6*MAXGEN +1),MGENP), (MGEN(7*MAXGEN +1),MEXSAT),
     4  (MGEN(8*MAXGEN +1),MREG), (MGEN(9*MAXGEN +1),MPACC),
     5  (MGEN(10*MAXGEN +1),MGENQ), (MGEN(11*MAXGEN +1),MSUP),
     6  (MGEN(12*MAXGEN +1),MTQDP), (MGEN(13*MAXGEN +1),MFCUR),
     7  (MGEN(14*MAXGEN +1),MPA), (MGEN(15*MAXGEN +1),MCUR)
