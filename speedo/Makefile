CC = gcc
CFLAGS = -O2 -Wall -Wstrict-prototypes -Wmissing-prototypes -I../include
LDFLAGS = -s
LIBS= -lX11

PROGRAMS = htest nsample sptobdf listids

all: $(PROGRAMS)


.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

htest:	htest.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

SPDLIB = reset.o do_char.o do_trns.o set_spcs.o set_trns.o \
	out_bl2d.o out_blk.o out_scrn.o out_util.o keys.o

# spencode.o spfuncs.o spfile.o spfont.o spglyph.o

nsample: nsample.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lm
	
sptobdf: sptobdf.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lm
	
listids: listids.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lm
	

clean::
	rm -f $(PROGRAMS) *.o
