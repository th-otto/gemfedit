CC = gcc
CFLAGS = -O2 -Wall -I../include
LDFLAGS = -s
LIBS= -lX11

PROGRAMS = fontdisp

all: $(PROGRAMS)

fontdisp: fontdisp.o decode.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

fontdisp.o: fontdisp.c
	$(CC) $(CFLAGS) $< -c -o $@

deocde.o: decode.c
	$(CC) $(CFLAGS) $< -c -o $@

win32: fontdisp.exe

fontdisp.exe:
	i686-pc-mingw32-gcc $(CFLAGS) -c fontdisp.c
	i686-pc-mingw32-gcc $(CFLAGS) -c decode.c
	i686-pc-mingw32-gcc $(LDFLAGS) -o $@ fontdisp.o decode.o -lgdi32 -lcomdlg32

clean::
	rm -f $(PROGRAMS) fontdisp.exe *.o
