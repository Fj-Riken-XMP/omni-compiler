/*
 * $TSUKUBA_Release: $
 * $TSUKUBA_Copyright:
 *  $
 */
#ifndef _XMP_DATA_STRUCT
#define _XMP_DATA_STRUCT
#include <mpi.h>
#include <stdbool.h>
#include "xmp_constant.h"
#if defined(OMNI_TARGET_CPU_KCOMPUTER) && defined(K_RDMA_REFLECT)
#include <mpi-ext.h>
#endif

#define _XMP_comm_t void

// nodes descriptor
typedef struct _XMP_nodes_inherit_info_type {
  int shrink;
  // enable when shrink is false
  int lower;
  int upper;
  int stride;
  // ---------------------------

  int size;
} _XMP_nodes_inherit_info_t;

typedef struct _XMP_nodes_info_type {
  int size;

  // enable when is_member is true
  int rank;
  // -----------------------------
  int multiplier;
} _XMP_nodes_info_t;

typedef struct _XMP_nodes_type {
  unsigned long long on_ref_id;

  int is_member;
  int dim;
  int comm_size;

  // enable when is_member is true
  int comm_rank;
  _XMP_comm_t *comm;
  // -----------------------------

  struct _XMP_nodes_type *inherit_nodes;
  // enable when inherit_nodes is not NULL
  _XMP_nodes_inherit_info_t *inherit_info;
  // -------------------------------------
  _XMP_nodes_info_t info[1];
} _XMP_nodes_t;

typedef struct _XMP_nodes_ref_type {
  _XMP_nodes_t *nodes;
  int *ref;
  int shrink_nodes_size;
} _XMP_nodes_ref_t;

// template desciptor
typedef struct _XMP_template_info_type {
  // enable when is_fixed is true
  long long ser_lower;
  long long ser_upper;
  unsigned long long ser_size;
  // ----------------------------
} _XMP_template_info_t;

typedef struct _XMP_template_chunk_type {
  // enable when is_owner is true
  long long par_lower;
  long long par_upper;
  unsigned long long par_width;
  // ----------------------------

  int par_stride;
  unsigned long long par_chunk_width;
  int dist_manner;
  _Bool is_regular_chunk;

  // enable when dist_manner is not _XMP_N_DIST_DUPLICATION
  int onto_nodes_index;
  // enable when onto_nodes_index is not _XMP_N_NO_ONTO_NODES
  _XMP_nodes_info_t *onto_nodes_info;
  // --------------------------------------------------------
} _XMP_template_chunk_t;

typedef struct _XMP_template_type {
  unsigned long long on_ref_id;

  _Bool is_fixed;
  _Bool is_distributed;
  _Bool is_owner;
  
  int   dim;

  // enable when is_distributed is true
  _XMP_nodes_t *onto_nodes;
  _XMP_template_chunk_t *chunk;
  // ----------------------------------

  _XMP_template_info_t info[1];
} _XMP_template_t;

// schedule of reflect comm.
typedef struct _XMP_reflect_sched_type {

  int lo_width, hi_width;
  _Bool is_periodic;

  MPI_Datatype datatype_lo;
  MPI_Datatype datatype_hi;

  MPI_Request req[4];

  void *lo_send_buf, *lo_recv_buf;
  void *hi_send_buf, *hi_recv_buf;

  void *lo_send_array, *lo_recv_array;
  void *hi_send_array, *hi_recv_array;

  int count, blocklength, stride;

  int lo_rank, hi_rank;

} _XMP_reflect_sched_t;

// aligned array descriptor
typedef struct _XMP_array_info_type {
  _Bool is_shadow_comm_member;
  _Bool is_regular_chunk;
  int align_manner;

  int ser_lower;
  int ser_upper;
  int ser_size;

  // enable when is_allocated is true
  int par_lower;
  int par_upper;
  int par_stride;
  int par_size;

  int local_lower;
  int local_upper;
  int local_stride;
  int alloc_size;

  int *temp0;
  int temp0_v;

  unsigned long long dim_acc;
  unsigned long long dim_elmts;
  // --------------------------------

  long long align_subscript;

  int shadow_type;
  int shadow_size_lo;
  int shadow_size_hi;

  _XMP_reflect_sched_t *reflect_sched;

  // enable when is_shadow_comm_member is true
  _XMP_comm_t *shadow_comm;
  int shadow_comm_size;
  int shadow_comm_rank;
  // -----------------------------------------

  int align_template_index;
} _XMP_array_info_t;

typedef struct _XMP_array_type {
  _Bool is_allocated;
  _Bool is_align_comm_member;
  int dim;
  int type;
  size_t type_size;

  // enable when is_allocated is true
  void *array_addr_p;
#if defined(OMNI_TARGET_CPU_KCOMPUTER) && defined(K_RDMA_REFLECT)
  uint64_t rdma_addr;
  int rdma_memid;
#endif
  unsigned long long total_elmts;
  // --------------------------------

  // FIXME do not use these members
  // enable when is_align_comm_member is true
  _XMP_comm_t *align_comm;
  int align_comm_size;
  int align_comm_rank;
  // ----------------------------------------

  //int num_reqs;
  //MPI_Request *mpi_req_shadow;

  _XMP_nodes_t *array_nodes;

  _XMP_template_t *align_template;
  _XMP_array_info_t info[1];
} _XMP_array_t;

typedef struct _XMP_task_desc_type {
  _XMP_nodes_t *nodes;
  int execute;

  unsigned long long on_ref_id;

  int ref_lower[_XMP_N_MAX_DIM];
  int ref_upper[_XMP_N_MAX_DIM];
  int ref_stride[_XMP_N_MAX_DIM];
} _XMP_task_desc_t;

typedef struct xmp_coarray{
  char **addr;      // Pointer on each node. The number of elements is process size.
                    // xmp_coarray.addr[2] is a pointer of real object on node 2.
  size_t elmt_size;
  int coarray_dims;
  long long *size;
  long long *distance_of_array_elmt;
  int image_dims;
  int *distance_of_image_elmt;
}_XMP_coarray_t;

typedef struct _XMP_array_section{
  long long start;
  long long length;
  long long stride;
  long long size;
  long long distance;
} _XMP_array_section_t;

typedef struct _XMP_gpu_array_type {
  int gtol;
  unsigned long long acc;
} _XMP_gpu_array_t;

typedef struct _XMP_gpu_data_type {
  _Bool is_aligned_array;
  void *host_addr;
  void *device_addr;
  _XMP_array_t *host_array_desc;
  _XMP_gpu_array_t *device_array_desc;
  size_t size;
} _XMP_gpu_data_t;

#endif // _XMP_DATA_STRUCT
