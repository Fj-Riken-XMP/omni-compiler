! main program
      !! src: libxmpf/src/xmpf_misc.c
      !! incl. atexit, _XMP_init, FJMPI_Rdma_init, etc.
      call xmpf_init_all_

      !! This subroutine is automatically generated.
      !! see Driver/src/xmp_collect_init.c
      call xmpf_module_init_

      !! This subroutine is sutomatically generated.
      !! see Driver/bin/omni_traverse.in
      call xmpf_traverse_initcoarray

      !! user's main program
      call xmpf_main

      !! src: libxmpf/src/xmpf_misc.c
      !! incl. FJMPI_Rdma_finalize(), _XMP_finalize, etc.
      call xmpf_finalize_all_
      end
