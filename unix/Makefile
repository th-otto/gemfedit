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

clean::
	rm -f $(PROGRAMS) *.o
