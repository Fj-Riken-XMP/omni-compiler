! testp129.f
! bcast�ؼ�ʸ�Υƥ��ȡ�from��template-ref��on��node-ref������ʬ�Ρ���

      program main
      include 'xmp_lib.h'
      integer,parameter:: N=1000
!$xmp nodes p(*)
!$xmp template t(N,N,N)
!$xmp distribute t(*,block,*) onto p
      integer aa(N), a
      real*4 bb(N), b
      real*8 cc(N), c
      integer procs, ans, w, id
      character(len=2) result

      id = xmp_node_num()
      procs = xmp_num_nodes()
      if(mod(N, procs) .eq. 0) then
         w = N/procs
      else
         w = N/procs+1
      endif

      result = 'OK'
      do j=1, N
         a = xmp_node_num()
         b = real(a)
         c = dble(a)
         do i=1, N
            aa(i) = a+i-1
            bb(i) = real(a+i-1)
            cc(i) = dble(a+i-1)
         enddo

!$xmp bcast (a) from t(:,j,:) on p(2:procs-1)
!$xmp bcast (b) from t(:,j,:) on p(2:procs-1)
!$xmp bcast (c) from t(:,j,:) on p(2:procs-1)
!$xmp bcast (aa) from t(:,j,:) on p(2:procs-1)
!$xmp bcast (bb) from t(:,j,:) on p(2:procs-1)
!$xmp bcast (cc) from t(:,j,:) on p(2:procs-1)

         if(id .ge. 2 .and. id .le. procs-1) then
            ans = (j-1)/w+1
            if(a .ne. ans) result = 'NG'
            if(b .ne. real(ans)) result = 'NG'
            if(c .ne. dble(ans)) result = 'NG'
            do i=1, N
               if(aa(i) .ne. ans+i-1) result = 'NG'
               if(bb(i) .ne. real(ans+i-1)) result = 'NG'
               if(cc(i) .ne. dble(ans+i-1)) result = 'NG'
            enddo
         else
            if(a .ne. xmp_node_num()) result = 'NG'
            if(b .ne. real(a)) result = 'NG'
            if(c .ne. dble(a)) result = 'NG'
            do i=1, N
               if(aa(i) .ne. a+i-1) result = 'NG'
               if(bb(i) .ne. real(a+i-1)) result = 'NG'
               if(cc(i) .ne. dble(a+i-1)) result = 'NG'
            enddo
         endif
      enddo

      print *, xmp_node_num(), 'testp128.f ', result

      end
