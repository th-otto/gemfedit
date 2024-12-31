EVAL_CROSS := case `uname -s` in *MiNT*) echo no;; *) echo yes; esac
CROSS := $(shell $(EVAL_CROSS))

ifeq ($(CROSS),yes)

CROSSPREFIX=m68k-atari-mint-

else

CROSSPREFIX=

endif

NATIVECC = gcc
NATIVECFLAGS = -O -Wall

CC = $(CROSSPREFIX)gcc
AS = $(CC)
AR = $(CROSSPREFIX)ar
RANLIB = $(CROSSPREFIX)ranlib
STRIP = $(CROSSPREFIX)strip

CFLAGS = -O2 -Wall -I../include -I../speedo
LDFLAGS = -s
LIBS = -lgem
MATH_LIB = -lm

PROGRAMS = gemfedit.prg fontdisp.prg spdview.prg sptobdf.ttp fntconv.ttp

ifneq (,$(shell which $(CC) 2>/dev/null))
all: $(PROGRAMS)
else
all:
	@echo "$(CC) not found, not building for tos" >&2
endif


gemfedit.prg: gemfedit.o decode.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

fontdisp.prg: fontdisp.o decode.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

gemfedit.o: gemfedit.c gemfedit.h
	$(CC) $(CFLAGS) -c -o $@ $<

fontdisp.o: fontdisp.c fontdisp.h
	$(CC) $(CFLAGS) -c -o $@ $<

decode.o: decode.c fontdisp.h
	$(CC) $(CFLAGS) -c -o $@ $<

SPDLIB = reset.o do_char.o do_trns.o set_spcs.o set_trns.o \
	out_bl2d.o out_blk.o out_scrn.o out_util.o out_outl.o \
	keys.o sp_err.o bics2uni.o

sptobdf.ttp: sptobdf.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(MATH_LIB)

fntconv.ttp: fntconv.o ttfout.o array.o getopt.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(MATH_LIB)

spdview.prg: spdview.o $(SPDLIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) $(MATH_LIB)

sptobdf.o: ../speedo/sptobdf.c
	$(CC) $(CFLAGS) -c -o $@ $<

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

fntconv.o: ../tools/fntconv.c
	$(CC) $(CFLAGS) -c -o $@ $<

ttfout.o: ../tools/ttfout.c
	$(CC) $(CFLAGS) -c -o $@ $<

array.o: ../tools/array.c
	$(CC) $(CFLAGS) -c -o $@ $<


clean::
	rm -f *.o .out *.pdb $(PROGRAMS)
