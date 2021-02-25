CC = i686-pc-mingw32-gcc
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
