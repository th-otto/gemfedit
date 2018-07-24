CC = gcc

USE_PNG12=1
#USE_PNG14=1
#USE_PNG16=1

ifdef USE_PNG12
LIBPNG = -lpng12
PNGDEF = -DUSE_PNG12
endif
ifdef USE_PNG14
LIBPNG = -lpng14
PNGDEF = -DUSE_PNG14
endif
ifdef USE_PNG16
LIBPNG = -lpng16
PNGDEF = -DUSE_PNG16
endif
ifndef LIBPNG
LIBPNG = -lpng
endif

WARN = -fno-common -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes -Werror
CFLAGS = -O2 -g -fomit-frame-pointer $(WARN) $(PNGDEF) -I../include -I../speedo -I../freetype/include
LDFLAGS = -s
LIBS= -lcurl $(LIBPNG) -lz

SPDLIB = reset.o do_char.o do_trns.o set_spcs.o set_trns.o \
	out_bl2d.o out_blk.o out_scrn.o out_util.o out_outl.o \
	keys.o sp_err.o bics2uni.o

TTFLIB = 

PROGRAMS = spdview.cgi ttfview.cgi

all: $(PROGRAMS)

spdview.cgi: spdview.o cgic.o glib.o $(SPDLIB) writepng.o cgiutil.o ucd.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	cp $@ ../www

ttfview.cgi: ttfview.o cgic.o glib.o $(TTFLIB) writepng.o cgiutil.o ucd.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -L../freetype -lttf -lbz2 $(LIBS) -o $@
	cp $@ ../www

spdview.o: spdview.c
	$(CC) $(CFLAGS) $< -c -o $@

ttfview.o: ttfview.c
	$(CC) $(CFLAGS) $< -c -o $@

cgic.o: cgic.c
	$(CC) $(CFLAGS) $< -c -o $@

glib.o: glib.c
	$(CC) $(CFLAGS) $< -c -o $@

reset.o: ../speedo/reset.c
	$(CC) $(CFLAGS) -c -o $@ $<

do_char.o: ../speedo/do_char.c
	$(CC) $(CFLAGS) -c -o $@ $<

do_trns.o: ../speedo/do_trns.c
	$(CC) $(CFLAGS) -c -o $@ $<

set_spcs.o: ../speedo/set_spcs.c
	$(CC) $(CFLAGS) -c -o $@ $<

set_trns.o: ../speedo/set_trns.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_bl2d.o: ../speedo/out_bl2d.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_blk.o: ../speedo/out_blk.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_scrn.o: ../speedo/out_scrn.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_outl.o: ../speedo/out_outl.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_util.o: ../speedo/out_util.c
	$(CC) $(CFLAGS) -c -o $@ $<

keys.o: ../speedo/keys.c
	$(CC) $(CFLAGS) -c -o $@ $<

sp_err.o: ../speedo/sp_err.c
	$(CC) $(CFLAGS) -c -o $@ $<

bics2uni.o: ../speedo/bics2uni.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean::
	rm -f $(PROGRAMS) *.o
