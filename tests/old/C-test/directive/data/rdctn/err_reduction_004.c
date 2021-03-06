static char rcsid[] = "$Id$";
/* 
 * $TSUKUBA_Release: Omni OpenMP Compiler 3 $
 * $TSUKUBA_Copyright:
 *  PLEASE DESCRIBE LICENSE AGREEMENT HERE
 *  $
 */
/* error case of reduction 004 :
 * threadprivate 宣言された変数に対して reduction を宣言した場合
 */

#include <omp.h>
#include "omni.h"


int	errors = 0;
int	thds;


int	rdct;
#pragma omp threadprivate (rdct)


main ()
{
  thds = omp_get_max_threads ();
  if (thds == 1) {
    printf ("should be run this program on multi threads.\n");
    exit (0);
  }
  omp_set_dynamic (0);

  #pragma omp parallel
  {
    int	i;

    #pragma omp for reduction(+:rdct)
    for (i=0;i<thds;i++) {
      rdct += i;
    }
  }


  printf ("err_reduction 004 : FAILED, can not compile this program.\n");
  return 1;
}
