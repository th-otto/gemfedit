CC = gcc
CFLAGS = -O2 -Wall -I../include
LDFLAGS = -s
LIBS=

PROGRAMS = fnt2bdf

all: $(PROGRAMS)

fnt2bdf: fnt2bdf.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

clean::
	rm -f $(PROGRAMS)
