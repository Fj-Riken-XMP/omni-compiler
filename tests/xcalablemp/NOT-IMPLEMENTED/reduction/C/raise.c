#include<mpi.h>
#include<xmp.h>
#include<stdio.h> 
#include<stdlib.h>   
static const int N=1000;
#pragma xmp nodes p(*)
int random_array[1000],ans_val,val;
#pragma xmp template t(0:N-1)
#pragma xmp distribute t(block) onto p
char *result;
int i,k;
int main(void){

   result = "OK";
   for(k=114;k<10001;k=k+17){
      random_array[0] = k;
      for(i=1;i<N;i++){
         random_array[i]=(random_array[i-1]*random_array[i-1])%100000000;
         random_array[i]=(random_array[i]-((random_array[i]%100)/100))%10000;
      }
   
      ans_val = 0;
      for(i=0;i<N;i++)
         {
            ans_val = ans_val^random_array[i];
         }
      val = 0;
#pragma xmp loop on t(i)
      for(i=0;i<N;i++){
         val = val^random_array[i];
      }
#pragma xmp reduction(^:val)
      if(val != ans_val){
         printf("%d %s %d\n",val,"!=",ans_val);
         result = "NG";
      }
   }

   printf("%d %s %s\n",xmp_node_num(),"testp038.c",result); 
   return 0;
}
      
         
      
   
