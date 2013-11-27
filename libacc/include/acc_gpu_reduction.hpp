#ifndef _ACC_GPU_REDUCTION
#define _ACC_GPU_REDUCTION

#include <limits.h>
#include <float.h>
#include <stdio.h>
#include "acc_gpu_atomic.hpp"

#define _ACC_REDUCTION_PLUS 0
#define _ACC_REDUCTION_MUL 1
#define _ACC_REDUCTION_MAX 2
#define _ACC_REDUCTION_MIN 3
#define _ACC_REDUCTION_BITAND 4
#define _ACC_REDUCTION_BITOR 5
#define _ACC_REDUCTION_BITXOR 6
#define _ACC_REDUCTION_LOGAND 7
#define _ACC_REDUCTION_LOGOR 8

static __device__
void _ACC_gpu_init_reduction_var(int *var, int kind){
  switch(kind){
  case _ACC_REDUCTION_PLUS: *var = 0; return;
  case _ACC_REDUCTION_MUL: *var = 1; return;
  case _ACC_REDUCTION_MAX: *var = INT_MIN; return;
  case _ACC_REDUCTION_MIN: *var = INT_MAX; return;
  case _ACC_REDUCTION_BITAND: *var = ~0; return;
  case _ACC_REDUCTION_BITOR: *var = 0; return;
  case _ACC_REDUCTION_BITXOR: *var = 0; return;
  case _ACC_REDUCTION_LOGAND: *var = 1; return;
  case _ACC_REDUCTION_LOGOR: *var = 0; return;
  }
}

static __device__
void _ACC_gpu_init_reduction_var(float *var, int kind){
  switch(kind){
  case _ACC_REDUCTION_PLUS: *var = 0.0f; return;
  case _ACC_REDUCTION_MUL: *var = 1.0f; return;
  case _ACC_REDUCTION_MAX: *var = FLT_MIN; return;
  case _ACC_REDUCTION_MIN: *var = FLT_MAX; return;
  }
}
static __device__
void _ACC_gpu_init_reduction_var(double *var, int kind){
  switch(kind){
  case _ACC_REDUCTION_PLUS: *var = 0.0; return;
  case _ACC_REDUCTION_MUL: *var = 1.0; return;
  case _ACC_REDUCTION_MAX: *var = DBL_MIN; return;
  case _ACC_REDUCTION_MIN: *var = DBL_MAX; return;
  }
}

template<typename T>
static __device__
void _ACC_gpu_init_reduction_var_single(T *var, int kind){
  if(threadIdx.x == 0 && threadIdx.y == 0 && threadIdx.z == 0){
    _ACC_gpu_init_reduction_var(var, kind);
  }
}

static __device__
int op(int a, int b, int kind){
  switch(kind){
  case _ACC_REDUCTION_PLUS: return a + b;
  case _ACC_REDUCTION_MUL: return a * b;
  case _ACC_REDUCTION_MAX: return (a > b)? a : b;
  case _ACC_REDUCTION_MIN: return (a < b)? a : b;
  case _ACC_REDUCTION_BITAND: return a & b;
  case _ACC_REDUCTION_BITOR: return a | b;
  case _ACC_REDUCTION_BITXOR: return a ^ b;
  case _ACC_REDUCTION_LOGAND: return a && b;
  case _ACC_REDUCTION_LOGOR: return a || b;
  default: return a;
  }
}

template<typename T>
static __device__
T op(T a, T b, int kind){
  switch(kind){
  case _ACC_REDUCTION_PLUS: return a + b;
  case _ACC_REDUCTION_MUL: return a * b;
  case _ACC_REDUCTION_MAX: return (a > b)? a : b;
  case _ACC_REDUCTION_MIN: return (a < b)? a : b;
  default: return a;
  }
}


template<typename T>
static __device__ void warpReduce(volatile T sdata[64], int kind){
  sdata[threadIdx.x] = op(sdata[threadIdx.x], sdata[threadIdx.x + 32], kind);
  sdata[threadIdx.x] = op(sdata[threadIdx.x], sdata[threadIdx.x + 16], kind);
  sdata[threadIdx.x] = op(sdata[threadIdx.x], sdata[threadIdx.x + 8], kind);
  sdata[threadIdx.x] = op(sdata[threadIdx.x], sdata[threadIdx.x + 4], kind);
  sdata[threadIdx.x] = op(sdata[threadIdx.x], sdata[threadIdx.x + 2], kind);
  sdata[threadIdx.x] = op(sdata[threadIdx.x], sdata[threadIdx.x + 1], kind);
}

template<typename T>
static __device__
void reduceInBlock(T *resultInBlock, T resultInThread, int kind){
  __shared__ T tmp[64];

  if(threadIdx.x < 64){
    tmp[threadIdx.x] = resultInThread;
  }
  __syncthreads();
  
  unsigned int div64_q = threadIdx.x >> 6;
  unsigned int div64_m = threadIdx.x % 64;
  unsigned int max_q = ((blockDim.x - 1) >> 6) + 1;
  for(unsigned int i = 1; i < max_q; i++){
    if(i == div64_q){
      tmp[div64_m] = op(tmp[div64_m], resultInThread, kind);
    }
    __syncthreads();
  }

  if(threadIdx.x < 32) warpReduce(tmp, kind);

  if(threadIdx.x == 0){
    *resultInBlock = tmp[0];
  }
}

template<typename T>
__device__
static void reduceInGridDefault(T *resultInGrid, T resultInBlock, int kind, T *tmp, unsigned int *cnt){
  __shared__ bool isLastBlockDone;

  if(threadIdx.x == 0){
    tmp[blockIdx.x] = resultInBlock;
    __threadfence();
    
    //increase cnt after tmp is visible from all.
    unsigned int value = atomicInc(cnt, gridDim.x);
    isLastBlockDone = (value == (gridDim.x - 1));
  }
  __syncthreads();

  if(isLastBlockDone){
    T part_result;
    _ACC_gpu_init_reduction_var(&part_result, kind);
    for(int idx = threadIdx.x; idx < gridDim.x; idx += blockDim.x){
      part_result = op(part_result, tmp[idx], kind);
    }
    T result;
    reduceInBlock(&result, part_result, kind);
    if(threadIdx.x == 0){
      *resultInGrid = op(*resultInGrid, result, kind);
      *cnt = 0; //important!
    }
  }
}

__device__
static void reduceInGrid(int *resultInGrid, int resultInBlock, int kind, int *tmp, unsigned int *cnt){
  if(kind != _ACC_REDUCTION_MUL){
    //use atomic operation
    if(threadIdx.x == 0){
      switch(kind){
      case _ACC_REDUCTION_PLUS:
	atomicAdd(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_MAX:
	atomicMax(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_MIN:
	atomicMin(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_BITAND:
	atomicAnd(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_BITOR:
	atomicOr(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_BITXOR:
	atomicXor(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_LOGOR:
	if(resultInBlock) atomicOr(resultInGrid, ~0);//atomicCAS(resultInGrid, 0, resultInBlock);
	break;
      case _ACC_REDUCTION_LOGAND:
	if(! resultInBlock) atomicAnd(resultInGrid, 0);break;
      }
    }
    __syncthreads(); //this is important
  }else{ //kind == _ACC_REDUCTION_MUL
    reduceInGridDefault(resultInGrid, resultInBlock, kind, tmp, cnt);
  }
}

__device__
static void reduceInGrid(float *resultInGrid, float resultInBlock, int kind, float *tmp, unsigned int *cnt){
  if(kind != _ACC_REDUCTION_MUL){
    //use atomic operation
    if(threadIdx.x == 0){
      switch(kind){
      case _ACC_REDUCTION_PLUS:
	atomicAdd(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_MAX:
	atomicMax(resultInGrid, resultInBlock);break;
      case _ACC_REDUCTION_MIN:
	atomicMin(resultInGrid, resultInBlock);break;
      }
    }
    __syncthreads();
  }else{ //kind == _ACC_REDUCTION_MUL
    reduceInGridDefault(resultInGrid, resultInBlock, kind, tmp, cnt);
  }
}

__device__
static void reduceInGrid(double *resultInGrid, double resultInBlock, int kind, double *tmp, unsigned int *cnt){
  if(kind == _ACC_REDUCTION_MAX){
    if(threadIdx.x == 0) atomicMax(resultInGrid, resultInBlock);
    __syncthreads();
  }else if(kind == _ACC_REDUCTION_MIN){
    if(threadIdx.x == 0) atomicMin(resultInGrid, resultInBlock);
    __syncthreads();
  }else{
    reduceInGridDefault(resultInGrid, resultInBlock, kind, tmp, cnt);
    //    __syncthreads();
  }
}  


template<typename T>
__device__
void _ACC_gpu_reduce_block_thread_x(T *result, T resultInThread, int kind){
  T resultInBlock;
  reduceInBlock(&resultInBlock, resultInThread, kind);
  reduceInGrid(result, resultInBlock, kind, NULL, NULL);
}

template<typename T>
__device__
void _ACC_gpu_reduce_block_thread_x(T *result, T resultInThread, int kind, T *tmp, unsigned int *cnt){
  T resultInBlock;
  reduceInBlock(&resultInBlock, resultInThread, kind);
  reduceInGrid(result, resultInBlock, kind, tmp, cnt);
}

template<typename T>
__device__
void _ACC_gpu_reduce_thread_x(T *result, T resultInThread, int kind){
  reduceInBlock(result, resultInThread, kind);
}

/////////////


//template<typename T>
static __device__
void _ACC_gpu_init_block_reduction(unsigned int *counter, void * volatile * tmp, size_t totalElementSize){
  // initial value of counter and *tmp is 0 or NULL.
  if(threadIdx.x == 0){
    if(*tmp == NULL){
      unsigned int value = atomicCAS(counter, 0, 1);
      //      printf("counter=%d(%d)\n",value,blockIdx.x);
      if(value == 0){
	//malloc
	*tmp = malloc(gridDim.x * totalElementSize);
	__threadfence();
	*counter = 0;
	//	printf("malloced (%d)\n",blockIdx.x);
      }else{
	//wait completion of malloc
	while(*tmp == NULL); //do nothing
      }
    }
  }
  __syncthreads();
}

template<typename T>
__device__
void _ACC_gpu_reduction_thread(T *resultInBlock, T resultInThread, int kind){
  reduceInBlock(resultInBlock, resultInThread, kind);
}

__device__
void _ACC_gpu_is_last_block(int *is_last, unsigned int *counter){
  __threadfence();
  if(threadIdx.x==0){
    unsigned int value = atomicInc(counter, gridDim.x);
    *is_last = (value == (gridDim.x - 1));
  }
  __syncthreads();
}

template<typename T>
__device__
static void reduceInGridDefault_new(T *result, T *tmp, int kind){
  T part_result;
  _ACC_gpu_init_reduction_var(&part_result, kind);
  for(int idx = threadIdx.x; idx < gridDim.x; idx += blockDim.x){
    part_result = op(part_result, tmp[idx], kind);
  }
  T resultInGrid;
  reduceInBlock(&resultInGrid, part_result, kind);
  if(threadIdx.x == 0){
    *result = op(*result, resultInGrid, kind);
  }
}

template<typename T>
__device__
void _ACC_gpu_reduction_block(T* result, int kind, void* tmp, size_t offsetElementSize){
  void *tmpAddr = (char*)tmp + (gridDim.x * offsetElementSize);
  reduceInGridDefault_new(result, (T*)tmpAddr, kind);
}

//template<typename T>
__device__
void _ACC_gpu_finalize_reduction(unsigned int *counter, void* tmp){
  *counter = 0;
  free(tmp);
}

template<typename T>
__device__
void _ACC_gpu_reduction_tmp(T resultInBlock, void *tmp, size_t offsetElementSize){
  if(threadIdx.x==0){
    void *tmpAddr =  (char*)tmp + (gridDim.x * offsetElementSize);
    ((T*)tmpAddr)[blockIdx.x] = resultInBlock;
  }
  __syncthreads();//is need?
}

#endif //_ACC_GPU_REDUCTION