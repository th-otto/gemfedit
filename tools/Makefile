CC = gcc
WARN = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -Wc++-compat -Wdeclaration-after-statement -Wc90-c99-compat
CFLAGS = -O2 -fomit-frame-pointer $(WARN) -I../include
LDFLAGS = -s
LIBS=

PROGRAMS = fnt2bdf pcf2bdf fntconv

all: $(PROGRAMS)

fnt2bdf: fnt2bdf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

pcf2bdf: pcf2bdf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

fntconv: fntconv.c array.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

clean::
	rm -f $(PROGRAMS)
