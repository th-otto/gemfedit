EVAL_MINGWCC := case `uname -s` in CYGWIN*) echo i686-w64-mingw32-gcc ;; MINGW*) echo gcc ;; *) echo i686-pc-mingw32-gcc;; esac
CC = $(shell $(EVAL_MINGWCC))
CFLAGS = -mwin32 -O2 -Wall -I../libpng -I../zlib -I../include
CPPFLAGS = -DHAVE_PNG_H -DZLIB_STATIC
LDFLAGS = -s -mwindows
LIBS= -lgdi32 -lcomdlg32

PROGRAMS = fontdisp.exe

all: $(PROGRAMS)

fontdisp.exe: fontdisp.o decode.o pnglib.o zlib.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

fontdisp.o: fontdisp.c ../unix/fontdisp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

decode.o: decode.c ../unix/decode.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -c -o $@

clean::
	rm -f $(PROGRAMS) *.o
