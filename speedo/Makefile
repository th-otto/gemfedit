CC = gcc
WARN = -fno-common -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes -Werror
CFLAGS = -O2 -g -fomit-frame-pointer $(WARN) -I../include
LDFLAGS = -s

PROGRAMS = htest nsample sptobdf listids spdinfo

all: $(PROGRAMS)


.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

htest:	htest.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

spdinfo:	spdinfo.o keys.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

SPDLIB = reset.o do_char.o do_trns.o set_spcs.o set_trns.o \
	out_bl2d.o out_blk.o out_scrn.o out_util.o out_outl.o \
	keys.o sp_err.o bics2uni.o

# spencode.o spfuncs.o spfile.o spfont.o spglyph.o

nsample: nsample.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lm
	
sptobdf: sptobdf.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lm
	
listids: listids.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lm
	

clean::
	rm -f $(PROGRAMS) *.o
