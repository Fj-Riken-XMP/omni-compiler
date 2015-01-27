#include <stdio.h>
#include <math.h>

extern int chk_int(int ierr);

int n=10;
double a[n][n],b[n][n];
#pragma xmp nodes p(2)
#pragma xmp template tx(0:n-1)
#pragma xmp distribute tx(block) onto p
#pragma xmp align a[i][*] with tx(i)
#pragma xmp align b[*][i] with tx(i)

int main(){

  int i,j,ierr;
  double err;

#pragma xmp loop (i) on tx(i)
  for(i=0;i<n;i++){
    for(j=0;j<n;j++){
      a[i][j]=i+j+2;
    }
  }

  for(i=0;i<n;i++){
#pragma xmp loop (j) on tx(j)
    for(j=0;j<n;j++){
      b[i][j]=0;
    }
  }

#pragma xmp gmove
  b[0:n][0:n]=a[0:n][0:n];

  err=0.0;
  for(i=0;i<n;i++){
#pragma xmp loop (j) on tx(j)
    for(j=0;j<n;j++){
      err=err+fabs(b[i][j]-(i+j+2));
    }
  }

#pragma xmp reduction (MAX:err)
  ierr=err;
  chk_int(ierr);

}
