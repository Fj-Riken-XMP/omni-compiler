# clinpack.c make file.
# There are four cases to consider:
# gcc -DUNIX -DDP -DROLL
# gcc -DUNIX -DDP -DUNROLL
# gcc -DUNIX -DSP -DROLL
# gcc -DUNIX -DSP -DUNROLL
 
FILES = Makefile clinpack.c clinpack.bat

PROJDIR	= $(shell pwd)/../..
INCDIR	= $(PROJDIR)/include
TMPDIR	= .
CC		= /home/mnakao/work/xmp-trunk/bin/omc2c
CFLAGS	= -DGTODay -DNTIMES=100 -DORDER=100 \
	-O -Topenmp -I$(INCDIR) \
	--tempdir $(TMPDIR) --fixed-tempdir --stay-tempdir \
	--Wf--no-warn-W002 --Wf--no-warn-W003  --Wf--no-warn-W018 --Wx-dxcode
LDFLAGS	= -Topenmp
LDLIBS	= -lm

clinpack        : clinpack.c
		$(CC) $(CFLAGS) -DDP -DROLL clinpack.c -o clinpack_dpr $(LDLIBS)
		$(CC) $(CFLAGS) -DDP -DUNROLL clinpack.c -o clinpack_dpu $(LDLIBS)
		$(CC) $(CFLAGS) -DSP -DROLL clinpack.c -o clinpack_spr $(LDLIBS)
		$(CC) $(CFLAGS) -DSP -DUNROLL clinpack.c -o clinpack_spu $(LDLIBS)

clean:
		rm -f clinpack_[ds]p[ur] *.o *~

MANIFEST	: $(FILES)
		ls -l $(FILES) > MANIFEST

shar		: $(FILES) MANIFEST
		shar -o clinpack.shar MANIFEST $(FILES)

tar		: $(FILES) MANIFEST
		tar -cf clinpack.tar MANIFEST  $(FILES)
		compress clinpack.tar
