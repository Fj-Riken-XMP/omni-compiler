#define TRUE 1
#define FALSE 0
#include <stdio.h>
#include <string.h>
#include <xmp_api.h>
#include "xmp.h"

int img_dims[1];
// int a[10]:[*], a_test[10];
int (*a_p)[10];
xmp_desc_t a_desc;
int a_test[10];

// float b[3][5]:[*], b_test[3][5];
float (*b_p)[3][5];
xmp_desc_t b_desc;
float b_test[3][5];

// double c[2][3][4]:[*], c_test[2][3][4];
double (*c_p)[2][3][4];
xmp_desc_t c_desc;
double c_test[2][3][4];

// long d[3][4][3][2]:[*], d_test[3][4][3][2];
long (*d_p)[3][4][3][2];
xmp_desc_t d_desc;
long d_test[3][4][3][2];

// int e[5][7]:[*], e_test[5][7];
int (*e_p)[5][7];
xmp_desc_t e_desc;
int e_test[5][7];

int return_val = 0;
// #pragma xmp nodes p[2]

void initialize(int me){
  int i, j, m, n, t = me * 100;
  
  for(i=0;i<10;i++){
    // a[i] = i + t;
    // a_test[i] = a[i];
    (*a_p)[i] = i + t;
    a_test[i] = (*a_p)[i];
  }
  
  for(i=0;i<3;i++){
    for(j=0;j<5;j++){
      // b[i][j] = 5*i + j + t;
      // b_test[i][j] = b[i][j];
      (*b_p)[i][j] = 5*i + j + t;
      b_test[i][j] = (*b_p)[i][j];
    }
  }
	
  for(i=0;i<2;i++){
    for(j=0;j<3;j++){
      for(m=0;m<4;m++){
	// c[i][j][m] = 12*i + 4*j + m + t;
	// c_test[i][j][m] = c[i][j][m];
	      (*c_p)[i][j][m] = 12*i + 4*j + m + t;
	      c_test[i][j][m] = (*c_p)[i][j][m];
      }
    }
  }
  
  for(i=0;i<3;i++){
    for(j=0;j<4;j++){
      for(m=0;m<3;m++){
	// for(n=0;n<2;n++){
	//   d[i][j][m][n] = 12*i + 6 * j + 2 * m + n + t;
	//   d_test[i][j][m][n] = d[i][j][m][n];
	// }
	      for(n=0;n<2;n++){
	        (*d_p)[i][j][m][n] = 12*i + 6 * j + 2 * m + n + t;
	        d_test[i][j][m][n] = (*d_p)[i][j][m][n];
	      }
      }
    }
  }

  for(i=0;i<5;i++){
    for(j=0;j<7;j++){
      // e[i][j] = 9*i + j + t;
      // e_test[i][j] = e[i][j];
      (*e_p)[i][j] = 9*i + j + t;
      e_test[i][j] = (*e_p)[i][j];
    }
  }
}

void communicate_1(int me){
  xmp_array_section_t *a_section;
  xmp_sync_all(NULL);

  if(me == 1){
    int tmp[100];
    long tmp_dims[1];
    xmp_local_array_t *tmp_local;
    xmp_array_section_t *tmp_l_section;

    a_section = xmp_new_array_section(1);
    tmp_l_section = xmp_new_array_section(1);

    xmp_array_section_set_triplet(a_section,0,2,5,1);
    xmp_array_section_set_triplet(tmp_l_section,0,3,5,1);

    tmp_dims[0] = 100;    
    tmp_local = xmp_new_local_array(sizeof(int), 1, tmp_dims, (void **)&tmp);

    // tmp[3:5] = a[2:5]:[0]; // get
    img_dims[0] = 0;
    xmp_coarray_get_local(img_dims,a_desc,a_section,tmp_local,tmp_l_section);

    // memcpy(&a[3], &tmp[3], sizeof(int)*5);
    memcpy(&(*a_p)[3], &tmp[3], sizeof(int)*5);

		xmp_free_array_section(a_section);
		xmp_free_array_section(tmp_l_section);
  }

  if(me == 0){
    int tmp[50];
    long tmp_dims[1];
    xmp_local_array_t *tmp_local;
    xmp_array_section_t *tmp_l_section;

    a_section = xmp_new_array_section(1);
    tmp_l_section = xmp_new_array_section(1);
    xmp_array_section_set_triplet(a_section,0,0,2,1);
    xmp_array_section_set_triplet(tmp_l_section,0,8,2,1);

    tmp_dims[0] = 50;
    tmp_local = xmp_new_local_array(sizeof(int), 1, tmp_dims, (void **)&tmp);

    tmp[8] = 999; tmp[9] = 1000;

    // a[0:2]:[1] = tmp[8:2]; // put
    img_dims[0] = 1;
    xmp_coarray_put_local(img_dims,a_desc,a_section,tmp_local,tmp_l_section);

		xmp_free_array_section(a_section);
		xmp_free_array_section(tmp_l_section);
  }

  if(me == 1){
    a_test[3] = 2; a_test[4] = 3; a_test[5] = 4;
    a_test[6] = 5; a_test[7] = 6; 
    a_test[0] = 999; a_test[1] = 1000;
  }
  
  xmp_sync_all(NULL);
}

void check_1(int me){
  int i, flag = TRUE;
  
  for(i=0; i<10; i++){
    if( (*a_p)[i] != a_test[i] ){
      flag = FALSE;
      printf("[%d] (*a_p)[%d] check_1 : fall\ta[%d] = %d (True value is %d)\n",
	     me, i, i, (*a_p)[i], a_test[i]);
    }
  }
  xmp_sync_all(NULL);
  if(flag == TRUE)   printf("[%d] check_1 : PASS\n", me);
  else return_val = 1;
}

void communicate_2(int me){
  xmp_sync_all(NULL);

  if(me == 0){
    xmp_array_section_t *b_remote_section;
    xmp_array_section_t *b_local_section;
    int a1, a2, a3, a4, a5, a6, a7;

    a1 = 1; a2 = 2; a3 = 3;
    a4 = 0; a5 = 1; a6 = 3;
    a7 = 1;

    /* get process */
    // b[a4][a5:a6]:[a7]
    //   [0][1:3]:[1]
    b_remote_section = xmp_new_array_section(2);
		xmp_array_section_set_triplet(b_remote_section,0,a4,1,1);
		xmp_array_section_set_triplet(b_remote_section,1,a5,a6,1);

    // b[a1][a2:a3]
    // b[1][2:3]
    b_local_section = xmp_new_array_section(2);
		xmp_array_section_set_triplet(b_local_section,0,a1,1,1);
		xmp_array_section_set_triplet(b_local_section,1,a2,a3,1);

    // b[a1][a2:a3] = b[a4][a5:a6]:[a7];  // get
    img_dims[0] = a7;
    xmp_coarray_get(img_dims,b_desc,b_remote_section,b_desc,b_local_section);

		xmp_free_array_section(b_remote_section);
		xmp_free_array_section(b_local_section);

    /* put process */
    b_remote_section = xmp_new_array_section(2);
		xmp_array_section_set_triplet(b_remote_section,0,2,1,1);
		xmp_array_section_set_triplet(b_remote_section,1,0,1,1);

    b_local_section = xmp_new_array_section(2);
		xmp_array_section_set_triplet(b_local_section,0,0,1,1);
		xmp_array_section_set_triplet(b_local_section,1,0,1,1);
    
    // TODO: check [:1] width=1?
    // b[2][:1]:[1] = b[0][:1];           // put
    img_dims[0] = 1;
    xmp_coarray_put(img_dims,b_desc,b_remote_section,b_desc,b_local_section);

		xmp_free_array_section(b_remote_section);
		xmp_free_array_section(b_local_section);
  }
  
  if(me == 0){
    b_test[1][2] = 101; b_test[1][3] = 102; b_test[1][4] = 103;
  }
  if(me == 1){
    b_test[2][0] = 0;
  }
  
  xmp_sync_all(NULL);
}

void check_2(int me){
  xmp_sync_all(NULL);
  int i, j, flag = TRUE;
  
  for(i=0;i<3;i++){
    for(j=0;j<5;j++){
      if( (*b_p)[i][j] != b_test[i][j] ){
	flag = FALSE;
	printf("[%d] (*b_p)[%d][%d] check_2 : fall\tb[%d][%d] = %.f (True value is %.f)\n",
	       me, i, j, i, j, (*b_p)[i][j], b_test[i][j]);
      }
    }
  }
  xmp_sync_all(NULL);
  if(flag == TRUE)   printf("[%d] check_2 : PASS\n", me);
  else return_val = 1;
}

void communicate_3(int me){
  xmp_sync_all(NULL);
  if(me == 1){
    xmp_array_section_t *c_remote_section;
    xmp_array_section_t *c_local_section;

    c_remote_section = xmp_new_array_section(3);
		xmp_array_section_set_triplet(c_remote_section,0,1,1,1);
		xmp_array_section_set_triplet(c_remote_section,1,2,1,1);
		xmp_array_section_set_triplet(c_remote_section,2,0,1,1);

    c_local_section = xmp_new_array_section(3);
		xmp_array_section_set_triplet(c_local_section,0,1,1,1);
		xmp_array_section_set_triplet(c_local_section,1,1,1,1);
		xmp_array_section_set_triplet(c_local_section,2,1,1,1);

    img_dims[0] = 0;
    // c[1][2][0:1]:[0] = c[1][1][1];     // put
    xmp_coarray_put(img_dims,c_desc,c_remote_section,c_desc,c_local_section);
		xmp_free_array_section(c_remote_section);
		xmp_free_array_section(c_local_section);
  }
  if(me == 0){
    double tmp[2][5];
    long tmp_dims[2];
    xmp_array_section_t *c_remote_section;
    xmp_local_array_t *tmp_local;
    xmp_array_section_t *tmp_l_section;

    tmp_l_section = xmp_new_array_section(2);
    xmp_array_section_set_triplet(tmp_l_section,0,1,1,1);
    xmp_array_section_set_triplet(tmp_l_section,1,0,5,1);

    tmp_dims[0] = 2;
    tmp_dims[1] = 5;
    tmp_local = xmp_new_local_array(sizeof(double), 2, tmp_dims, (void **)&tmp);

    c_remote_section = xmp_new_array_section(3);
		xmp_array_section_set_triplet(c_remote_section,0,0,1,1);
		xmp_array_section_set_triplet(c_remote_section,1,2,1,1);
		xmp_array_section_set_triplet(c_remote_section,2,1,5,1);

    img_dims[0] = 1;
    // tmp[1][0:5] = c[0][2][1:5]:[1];       // get
    xmp_coarray_get_local(img_dims,c_desc,c_remote_section,tmp_local,tmp_l_section);

		xmp_free_array_section(c_remote_section);
		xmp_free_array_section(tmp_l_section);

    // memcpy(&c[0][2][1], &tmp[1][0], sizeof(double) * 3);
    memcpy(&(*c_p)[0][2][1], &tmp[1][0], sizeof(double) * 3);
  }
  
  if(me == 0){
    c_test[1][2][0] = 117;
    c_test[0][2][1] = 109; c_test[0][2][2] = 110; c_test[0][2][3] = 111;
  }
  
  xmp_sync_all(NULL);
}

void check_3(int me){
  xmp_sync_all(NULL);
  int i, j, m, flag = TRUE;

  for(i=0;i<2;i++){
    for(j=0;j<3;j++){
      for(m=0;m<4;m++){
	if( (*c_p)[i][j][m] != c_test[i][j][m] ){
	  flag = FALSE;
	  printf("[%d] (*c_p)[%d][%d][%d] check_3 : fall\tc[%d][%d][%d] = %.f (True value is %.f)\n",
		 me, i, j, m, i, j, m, (*c_p)[i][j][m], c_test[i][j][m]);
	}
      }
    }
  }
  xmp_sync_all(NULL);
  if(flag == TRUE)   printf("[%d] check_3 : PASS\n", me);
  else return_val = 1;
}

void communicate_4(int me){
  xmp_sync_all(NULL);

  if(me == 1){
    long tmp[2] = {5, 9};
    long tmp_dims[1];

    xmp_array_section_t *d_section;
    xmp_local_array_t *tmp_local;
    xmp_array_section_t *tmp_l_section;

    tmp_l_section = xmp_new_array_section(1);
    xmp_array_section_set_triplet(tmp_l_section,0,0,2,1);

    tmp_dims[0] = 2;
    tmp_local = xmp_new_local_array(sizeof(long), 1, tmp_dims, (void **)&tmp);

    d_section = xmp_new_array_section(4);
		xmp_array_section_set_triplet(d_section,0,1,1,1);
		xmp_array_section_set_triplet(d_section,1,2,1,1);
		xmp_array_section_set_triplet(d_section,2,1,1,1);
		xmp_array_section_set_triplet(d_section,3,0,2,1);

    // d[1][2][1][:]:[0] = tmp[:];          // put
    img_dims[0] = 0;
    xmp_coarray_put_local(img_dims,d_desc,d_section,tmp_local,tmp_l_section);
		xmp_free_array_section(d_section);
		xmp_free_array_section(tmp_l_section);
  }
  if(me == 1){
    xmp_array_section_t *d_remote_section;
    xmp_array_section_t *d_local_section;

    // d[0][2][1][:]:[0]
    d_remote_section = xmp_new_array_section(4);
		xmp_array_section_set_triplet(d_remote_section,0,0,1,1);
		xmp_array_section_set_triplet(d_remote_section,1,2,1,1);
		xmp_array_section_set_triplet(d_remote_section,2,1,1,1);
		xmp_array_section_set_triplet(d_remote_section,3,0,2,1);

    // d[0][1][1][:]  
    d_local_section = xmp_new_array_section(4);
		xmp_array_section_set_triplet(d_local_section,0,0,1,1);
		xmp_array_section_set_triplet(d_local_section,1,1,1,1);
		xmp_array_section_set_triplet(d_local_section,2,1,1,1);
		xmp_array_section_set_triplet(d_local_section,3,0,2,1);

    // d[0][1][1][:] = d[0][2][1][:]:[0];   // get 
    img_dims[0] = 0;
    xmp_coarray_get(img_dims,d_desc,d_remote_section,d_desc,d_local_section);
		xmp_free_array_section(d_local_section);
		xmp_free_array_section(d_remote_section);
  }

  if(me == 0){
    d_test[1][2][1][0] = 5; d_test[1][2][1][1] = 9;
  }
  if(me == 1){
    d_test[0][1][1][0] = 14; d_test[0][1][1][1] = 15;
  }

  xmp_sync_all(NULL);
}


void check_4(int me){
  xmp_sync_all(NULL);

  int i, j, m, n, flag = TRUE;
  for(i=0;i<3;i++){
    for(j=0;j<4;j++){
      for(m=0;m<3;m++){
	for(n=0;n<2;n++){
	  if( (*d_p)[i][j][m][n] != d_test[i][j][m][n] ){
	    flag = FALSE;
	    printf("[%d] d[%d][%d][%d][%d] check_3 : fall\td[%d][%d][%d][%d] = %ld (True value is %ld)\n",
		   me, i, j, m, n, i, j, m, n, (*d_p)[i][j][m][n], d_test[i][j][m][n]);
	  }
        }
      }
    }
  }

  xmp_sync_all(NULL);
  if(flag == TRUE)   printf("[%d] check_4 : PASS\n", me);
  else return_val = 1;
}

void communicate_5(int me){
  xmp_sync_all(NULL);
  const int e1 = 1, e2 = 2;
  const int e3 = 2, e4 = 3;

  if(me == 0){
    xmp_array_section_t *e_remote_section;
    xmp_array_section_t *e_local_section;

    e_remote_section = xmp_new_array_section(2);
    // e[2][3:2]:[1]; e[e1+1][e2+1:2]:[1] 
		xmp_array_section_set_triplet(e_remote_section,0,e1+1,1,1);
		xmp_array_section_set_triplet(e_remote_section,1,e2+1,2,1);

    e_local_section = xmp_new_array_section(2);
    // e[3][4:2]; e[e3+1][e4+1:2]
		xmp_array_section_set_triplet(e_local_section,0,e3+1,1,1);
		xmp_array_section_set_triplet(e_local_section,1,e4+1,2,1);

    // put 
    // e[e1+1][e2+1:2]:[1] = e[e3+1][e4+1:2];
    img_dims[0] = 1;
    xmp_coarray_put(img_dims,e_desc,e_remote_section,e_desc,e_local_section);

		xmp_free_array_section(e_remote_section);
		xmp_free_array_section(e_local_section);

    e_test[3][5] = 132;
    e_test[4][5] = 141;
  }

  if(me == 1){
    xmp_array_section_t *e_remote_section;
    xmp_array_section_t *e_local_section;

    e_remote_section = xmp_new_array_section(2);
    // e[3:2][e4+2]:[0]
		xmp_array_section_set_triplet(e_remote_section,0,3,2,1);
		xmp_array_section_set_triplet(e_remote_section,1,e4+2,1,1);

    e_local_section = xmp_new_array_section(2);
    //  e[3:2][e4+2]
		xmp_array_section_set_triplet(e_local_section,0,3,2,1);
		xmp_array_section_set_triplet(e_local_section,1,e4+2,1,1);

    // e[3:2][e4+2]:[0] = e[3:2][e4+2];
    // put 
    img_dims[0] = 0;
    xmp_coarray_put(img_dims,e_desc,e_remote_section,e_desc,e_local_section);

		xmp_free_array_section(e_remote_section);
		xmp_free_array_section(e_local_section);
    
    e_test[2][3] = 31;
    e_test[2][4] = 32;
  }
  xmp_sync_all(NULL);
}

void check_5(int me){
  xmp_sync_all(NULL);
  int i, j, flag = TRUE;

  for(i=0;i<5;i++){
    for(j=0;j<7;j++){
      if( (*e_p)[i][j] != e_test[i][j] ){
	flag = FALSE;
	printf("[%d] b[%d][%d] check_5 : fall\tb[%d][%d] = %d (True value is %d)\n",
	       me, i, j, i, j, (*e_p)[i][j], e_test[i][j]);
      }
    }
  }
  xmp_sync_all(NULL);
  if(flag == TRUE)   printf("[%d] check_5 : PASS\n", me);
  else return_val = 1;
}

void bug_107()
{
  
}

int main(int argc, char *argv[]){
  long a_dims[1];
  long b_dims[2];
  long c_dims[3];
  long d_dims[4];
  long e_dims[2];
  xmp_api_init(argc,argv);

  // For communicate_1
  a_dims[0] = 10;
  a_desc = xmp_new_coarray(sizeof(int), 1,a_dims,1,img_dims,(void **)&a_p);

  // For communicate_2
  b_dims[0] = 3;
  b_dims[1] = 5;
  b_desc = xmp_new_coarray(sizeof(float), 2,b_dims,1,img_dims,(void **)&b_p);

  // For communicate_3
  c_dims[0] = 2;
  c_dims[1] = 3;
  c_dims[2] = 4;
  c_desc = xmp_new_coarray(sizeof(double), 3,c_dims,1,img_dims,(void **)&c_p);

  // For communicate_4
  d_dims[0] = 3;
  d_dims[1] = 4;
  d_dims[2] = 3;
  d_dims[3] = 2;
  d_desc = xmp_new_coarray(sizeof(long), 4,d_dims,1,img_dims,(void **)&d_p);

  // For communicate_5
  e_dims[0] = 5;
  e_dims[1] = 7;
  e_desc = xmp_new_coarray(sizeof(int), 2,e_dims,1,img_dims,(void **)&e_p);

  int me = xmpc_this_image();
  initialize(me);
  
  communicate_1(me);
  check_1(me);
  
  communicate_2(me);
  check_2(me);
  
  communicate_3(me);
  check_3(me);
  
  communicate_4(me);
  check_4(me);

  communicate_5(me);
  check_5(me);

  bug_107();
  
  xmp_api_finalize();

// #pragma xmp barrier
// #pragma xmp reduction(MAX:return_val)
  return return_val;
}
