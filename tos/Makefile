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

CFLAGS = -O2 -Wall -I../include
LDFLAGS = -s
LIBS = -lgem

PROGRAMS = gemfedit.prg fontdisp.prg

all: $(PROGRAMS)


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


clean::
	rm -f *.o .out *.pdb $(PROGRAMS)
