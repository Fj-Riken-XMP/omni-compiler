#
OMNI_HOME = $(shell xmpcc --show-env | grep OMNI_HOME | sed -e 's/OMNI_HOME=//' -e 's/"//g')
CC     = cc
MPICC  = mpicc
CFLAGS = $(shell xmpcc --show-env | grep OMNI_MPICC_OPT | sed -e 's/OMNI_MPICC_OPT=//' -e 's/"//g')
LIBS = $(shell xmpcc --show-env | grep OMNI_LINKER_OPT | sed -e 's/OMNI_LINKER_OPT=//' -e 's/"//g')

all: xmp_api_laplace

xmp_api_laplace: xmp_api_laplace.o
	$(MPICC) -o xmp_api_laplace xmp_api_laplace.o $(LIBS)

xmp_api_laplace.o: xmp_api_laplace.c
	$(MPICC) $(CFLAGS) -c xmp_api_laplace.c
