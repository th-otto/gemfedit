CC = gcc
WARN = -fno-common -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes -Werror
CFLAGS = -O2 -g -fomit-frame-pointer $(WARN) -I. -I../include
LDFLAGS = -s

all: libz.a


.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

ZLIB = \
	adler32.o \
	crc32.o \
	deflate.o \
	infback.o \
	inffast.o \
	inflate.o \
	inftrees.o \
	trees.o \
	zutil.o \
	compress.o \
	uncompr.o \
	gzclose.o \
	gzlib.o \
	gzread.o \
	gzwrite.o \
	contrib/minizip/ioapi.o \
	contrib/minizip/zip.o \
	contrib/minizip/unzip.o \
	contrib/minizip/iowin32.o \
	$(empty)

match.o: match.S
	$(CPP) match.S > _match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s

libz.a: $(ZLIB)
	rm -f $@
	ar rcs $@ $^	

clean::
	rm -f *.o *.a contrib/minizip/*.o
