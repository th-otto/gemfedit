CC = gcc
WARN = -fno-common -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes -Werror
CFLAGS = -O2 -g -fomit-frame-pointer $(WARN) -I. -I../include -I../zlib
LDFLAGS = -s

all: libpng.a


.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

PNGLIB = \
	png.o \
	pngerror.o \
	pngget.o \
	pngmem.o \
	pngpread.o \
	pngread.o \
	pngrio.o \
	pngrtran.o \
	pngrutil.o \
	pngset.o \
	pngtrans.o \
	pngwio.o \
	pngwrite.o \
	pngwtran.o \
	pngwutil.o \
	intel/intel_init.o \
	intel/filter_sse2_intrinsics.o \
	$(empty)

libpng.a: $(PNGLIB)
	rm -f $@
	ar rcs $@ $^	

clean::
	rm -f *.o *.a
