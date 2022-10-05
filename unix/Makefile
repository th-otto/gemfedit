CC = gcc

CFLAGS = -O2 -Wall -I../include -I../libpng -DHAVE_PNG_H
LDFLAGS = -s
LIBS= -L../libpng -lX11 -lpng

PROGRAMS = fontdisp

all: $(PROGRAMS)

fontdisp: fontdisp.o decode.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

fontdisp.o: fontdisp.c
	$(CC) $(CFLAGS) $< -c -o $@

decode.o: decode.c
	$(CC) $(CFLAGS) $< -c -o $@

clean::
	rm -f $(PROGRAMS) fontdisp.exe *.o
