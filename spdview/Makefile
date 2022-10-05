CC = gcc

WARN = -fno-common -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes -Werror
CFLAGS = -O2 -g -fomit-frame-pointer $(WARN)
CPPFLAGS = -I../include -I../speedo -I../freetype/include
LDFLAGS = -s
LIBS = -lcurl

#USE_PNG12=1
#USE_PNG14=1
#USE_PNG16=1
USE_PNG_INCLUDED=1

ifdef USE_PNG12
LIBPNG = -lpng12
CPPFLAGS += -DUSE_PNG12
endif
ifdef USE_PNG14
LIBPNG = -lpng14
CPPFLAGS += -DUSE_PNG14
endif
ifdef USE_PNG16
LIBPNG = -lpng16
CPPFLAGS += -DUSE_PNG16
endif
ifdef USE_PNG_INCLUDED
LIBPNG = -L../libpng -lpng -lm
CPPFLAGS += -I../libpng
endif
ifndef LIBPNG
LIBPNG = -lpng
endif

LIBS += $(LIBPNG)
LIBS += -lz

SPDLIB = reset.o do_char.o do_trns.o set_spcs.o set_trns.o \
	out_bl2d.o out_blk.o out_scrn.o out_util.o out_outl.o \
	keys.o sp_err.o bics2uni.o

TTFLIB = ../freetype/libttf.a

PROGRAMS = spdview.cgi ttfview.cgi

all: $(PROGRAMS)

spdview.cgi: spdview.o cgic.o glib.o $(SPDLIB) writepng.o cgiutil.o ucd.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	cp $@ ../www

ttfview.cgi: ttfview.o cgic.o glib.o $(TTFLIB) writepng.o cgiutil.o ucd.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -L../freetype -lttf -lbz2 $(LIBS) -o $@
	cp $@ ../www

reset.o: ../speedo/reset.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

do_char.o: ../speedo/do_char.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

do_trns.o: ../speedo/do_trns.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

set_spcs.o: ../speedo/set_spcs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

set_trns.o: ../speedo/set_trns.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

out_bl2d.o: ../speedo/out_bl2d.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

out_blk.o: ../speedo/out_blk.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

out_scrn.o: ../speedo/out_scrn.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

out_outl.o: ../speedo/out_outl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

out_util.o: ../speedo/out_util.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

keys.o: ../speedo/keys.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

sp_err.o: ../speedo/sp_err.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

bics2uni.o: ../speedo/bics2uni.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean::
	rm -f $(PROGRAMS) *.o
