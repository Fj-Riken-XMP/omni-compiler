program reflect_array4d_dist3d
  integer, parameter :: lx = 6
  integer, parameter :: ly = 6
  integer, parameter :: lz = 6
  integer, parameter :: lt = 6

  !$XMP NODES px(2,1,1)
  !$XMP NODES py(1,2,1)
  !$XMP NODES pz(1,1,2)

  !$XMP TEMPLATE tx(lx,ly,lz)
  !$XMP TEMPLATE ty(lx,ly,lz)
  !$XMP TEMPLATE tz(lx,ly,lz)

  !$XMP DISTRIBUTE tx(BLOCK,BLOCK,BLOCK) ONTO px
  !$XMP DISTRIBUTE ty(BLOCK,BLOCK,BLOCK) ONTO py
  !$XMP DISTRIBUTE tz(BLOCK,BLOCK,BLOCK) ONTO pz

  real*8 :: array_x(lt,lx,ly,lz)
  real*8 :: array_y(lt,lx,ly,lz)
  real*8 :: array_z(lt,lx,ly,lz)

  !$XMP ALIGN (*,i,j,k) WITH tx(i,j,k) :: array_x
  !$XMP ALIGN (*,i,j,k) WITH ty(i,j,k) :: array_y
  !$XMP ALIGN (*,i,j,k) WITH tz(i,j,k) :: array_z

  !$XMP SHADOW (0,0:1,0:1,0:1) :: array_x
  !$XMP SHADOW (0,0:1,0:1,0:1) :: array_y
  !$XMP SHADOW (0,0:1,0:1,0:1) :: array_z

  integer :: err = 0

  !$xmp task on px(1,1,1)
  do it = 1, lt
     do ix = 1, 4 !1,2,3|4
        do iy = 1, ly
           do iz = 1, lz
              array_x(it,ix,iy,iz) = 1.0
           end do
        end do
     end do
  end do
  !$xmp end task
  !$xmp task on py(1,1,1)
  do it = 1, lt
     do ix = 1, lx
        do iy = 1, 4 !1,2,3|4
           do iz = 1, lz
              array_y(it,ix,iy,iz) = 1.0
           end do
        end do
     end do
  end do
  !$xmp end task
  !$xmp task on pz(1,1,1)
  do it = 1, lt
     do ix = 1, lx
        do iy = 1, ly
           do iz = 1, 4 !1,2,3|4
              array_z(it,ix,iy,iz) = 1.0
           end do
        end do
     end do
  end do
  !$xmp end task


  !$xmp task on px(2,1,1)
  do it = 1, lt
     do ix = 4, 6 !4,5,6|7
        do iy = 1, ly
           do iz = 1, lz
              array_x(it,ix,iy,iz) = 2.0
           end do
        end do
     end do
  end do
  !$xmp end task
  !$xmp task on py(1,2,1)
  do it = 1, lt
     do ix = 1, lx
        do iy = 4, 6 !4,5,6|7
           do iz = 1, lz
              array_y(it,ix,iy,iz) = 2.0
           end do
        end do
     end do
  end do
  !$xmp end task
  !$xmp task on pz(1,1,2)
  do it = 1, lt
     do ix = 1, lx
        do iy = 1, ly
           do iz = 4, 6 !4,5,6|7
              array_z(it,ix,iy,iz) = 2.0
           end do
        end do
     end do
  end do
  !$xmp end task

  !$acc data copy(array_x)
  !$XMP REFLECT (array_x) width(0,0:1,0,0) acc
  !$acc end data
  !$acc data copy(array_y)
  !$XMP REFLECT (array_y) width(0,0,0:1,0) acc
  !$acc end data
  !$acc data copy(array_z)
  !$XMP REFLECT (array_z) width(0,0,0,0:1) acc
  !$acc end data

  !$xmp task on px(1,1,1)
  do it = 1, lt
     do ix = 1, 4
        do iy = 1, ly
           do iz = 1, lz
              if(1 <= ix .and. ix <= 3) then
                 if(array_x(it,ix,iy,iz) /= 1.0) then
                    err = err + 1
                 endif
              else
                 if(array_x(it,ix,iy,iz) /= 2.0) then
                    err = err + 1
                 endif
              end if
           end do
        end do
     end do
  end do
  !$xmp end task

  !$xmp reduction(+:err)
  if(err /= 0) then
     call exit(1)
  end if

  !$xmp task on py(1,1,1)
  do it = 1, lt
     do ix = 1, lx
        do iy = 1, 4
           do iz = 1, lz
              if(1 <= iy .and. iy <= 3) then
                 if(array_y(it,ix,iy,iz) /= 1.0) then
                    err = err + 1
                 endif
              else
                 if(array_y(it,ix,iy,iz) /= 2.0) then
                    err = err + 1
                 endif
              end if
           end do
        end do
     end do
  end do
  !$xmp end task

  !$xmp reduction(+:err)
  if(err /= 0) then
     call exit(1)
  end if

  !$xmp task on pz(1,1,1)
  do it = 1, lt
     do ix = 1, lx
        do iy = 1, ly
           do iz = 1, 4
              if(1 <= iz .and. iz <= 3) then
                 if(array_z(it,ix,iy,iz) /= 1.0) then
                    err = err + 1
                 endif
              else
                 if(array_z(it,ix,iy,iz) /= 2.0) then
                    err = err + 1
                 endif
              end if
           end do
        end do
     end do
  end do
  !$xmp end task

  !$xmp reduction(+:err)
  if(err /= 0) then
     call exit(1)
  end if

  !$xmp task on px(1,1,1)
  write(*,*) "PASS"
  !$xmp end task

end program reflect_array4d_dist3d
