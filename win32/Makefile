CC = i686-pc-mingw32-gcc
CFLAGS = -mwin32 -O2 -Wall -I../include
LDFLAGS = -s
LIBS= -lgdi32 -lcomdlg32

PROGRAMS = fontdisp.exe

all: $(PROGRAMS)

fontdisp.exe: fontdisp.o decode.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

fontdisp.o: fontdisp.c
	$(CC) $(CFLAGS) $< -c -o $@

deocde.o: decode.c
	$(CC) $(CFLAGS) $< -c -o $@

clean::
	rm -f $(PROGRAMS) *.o
