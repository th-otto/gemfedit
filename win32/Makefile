EVAL_MINGWCC := case `uname -s` in CYGWIN*) echo i686-w64-mingw32-gcc ;; *) echo i686-pc-mingw32-gcc;; esac
CC = $(shell $(EVAL_MINGWCC))
CFLAGS = -mwin32 -O2 -Wall -I../include -DHAVE_PNG_H
LDFLAGS = -s -mwindows
LIBS= -lpng -lgdi32 -lcomdlg32

PROGRAMS = fontdisp.exe

all: $(PROGRAMS)

fontdisp.exe: fontdisp.o decode.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

fontdisp.o: fontdisp.c ../unix/fontdisp.c
	$(CC) $(CFLAGS) $< -c -o $@

decode.o: decode.c ../unix/decode.c
	$(CC) $(CFLAGS) $< -c -o $@

clean::
	rm -f $(PROGRAMS) *.o
