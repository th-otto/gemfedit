CC = gcc
CFLAGS = -O2 -fomit-frame-pointer -Wall -I../include
LDFLAGS = -s
LIBS=

PROGRAMS = fnt2bdf pcf2bdf fntconv

all: $(PROGRAMS)

fnt2bdf: fnt2bdf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

pcf2bdf: pcf2bdf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

fntconv: fntconv.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

clean::
	rm -f $(PROGRAMS)
