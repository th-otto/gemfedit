CC = gcc
CFLAGS = -O2 -Wall -I../include
LDFLAGS = -s
LIBS= -lcurl

all: spdview.cgi

spdview.cgi: spdview.o cgic.o glib.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	cp $@ ../www

spdview.o: spdview.c
	$(CC) $(CFLAGS) $< -c -o $@

cgic.o: cgic.c
	$(CC) $(CFLAGS) $< -c -o $@

glib.o: glib.c
	$(CC) $(CFLAGS) $< -c -o $@

clean::
	rm -f fontdisp *.o
