  program get1

    type g1
       integer*8 a(10)
       real*8    r(10)
    end type g1

    type(g1), static :: ns1, cs1[*]
    type(g1), static :: na1(3), ns1(3)[*]

    if (nerr==0) then 
       print '("result[",i0,"] OK")', me
    else
       print '("result[",i0,"] number of NGs: ",i0)', me, nerr
    end if
    !---------------------------- check and output end

  end program
