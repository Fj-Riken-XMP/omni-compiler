! testp138.f
! bcast�ؼ�ʸ�Υƥ��ȡ�from��node-ref��on��node-ref

      program main
      include 'xmp_lib.h'
      integer,parameter:: N=1000
!$xmp nodes p(4,*)
      integer aa(N), a
      real*4 bb(N), b
      real*8 cc(N), c
      integer procs, procs2, ans
      character(len=2) result

      procs = xmp_num_nodes()
      procs2 = procs/4

      result = 'OK'
      do k=1, procs2
         do j=1, 4
            a = xmp_node_num()
            b = real(a)
            c = dble(a)
            do i=1, N
               aa(i) = a+i-1
               bb(i) = real(a+i-1)
               cc(i) = dble(a+i-1)
            enddo

!$xmp bcast (a) from p(j,k) on p(:,:)
!$xmp bcast (b) from p(j,k) on p(:,:)
!$xmp bcast (c) from p(j,k) on p(:,:)
!$xmp bcast (aa) from p(j,k) on p(:,:)
!$xmp bcast (bb) from p(j,k) on p(:,:)
!$xmp bcast (cc) from p(j,k) on p(:,:)

            ans = (k-1)*4+j
            if(a .ne. ans) result = 'NG'
            if(b .ne. real(ans)) result = 'NG'
            if(c .ne. dble(ans)) result = 'NG'
            do i=1, N
               if(aa(i) .ne. ans+i-1) result = 'NG'
               if(bb(i) .ne. real(ans+i-1)) result = 'NG'
               if(cc(i) .ne. dble(ans+i-1)) result = 'NG'
            enddo
         enddo
      enddo
      
      print *, xmp_node_num(), 'testp138.f ', result

      end
