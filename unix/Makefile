EVAL_X11 := case `uname -s` in CYGWIN*|MINGW*) echo no ;; *) echo yes;; esac
X11 = $(shell $(EVAL_X11))
CC = gcc

CFLAGS = -O2 -Wall -I../include -I../libpng -DHAVE_PNG_H
LDFLAGS = -s
LIBS= -L../libpng -L../zlib -lX11 -lpng -lz -lm

ifeq ($(X11),yes)
PROGRAMS = fontdisp
endif

all: $(PROGRAMS)

fontdisp: fontdisp.o decode.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@ libc.so

fontdisp.o: fontdisp.c
	$(CC) $(CFLAGS) $< -c -o $@

decode.o: decode.c
	$(CC) $(CFLAGS) $< -c -o $@

clean::
	rm -f $(PROGRAMS) fontdisp.exe *.o
