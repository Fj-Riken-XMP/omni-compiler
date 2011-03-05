/*
 * $TSUKUBA_Release: $
 * $TSUKUBA_Copyright:
 *  $
 */

#include "mpi.h"
#include "xmp_internal.h"

int _XMP_world_size;
int _XMP_world_rank;
void *_XMP_world_nodes;

void _XMP_init_world(int *argc, char ***argv) {
  int flag = 0;
  MPI_Initialized(&flag);
  if (!flag) {
    MPI_Init(argc, argv);

    MPI_Comm *comm = _XMP_alloc(sizeof(MPI_Comm));
    MPI_Comm_dup(MPI_COMM_WORLD, comm);

    _XMP_nodes_t *n = _XMP_create_nodes_by_comm(comm);

    _XMP_world_size = n->comm_size;
    _XMP_world_rank = n->comm_rank;
    _XMP_world_nodes = n;

    _XMP_push_nodes(n);
  }

  MPI_Barrier(MPI_COMM_WORLD);
}

void _XMP_finalize_world(void) {
  MPI_Barrier(MPI_COMM_WORLD);

  int flag = 0;
  MPI_Finalized(&flag);
  if (!flag) {
    MPI_Finalize();
  }
}
