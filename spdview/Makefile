CC = gcc
CFLAGS = -O0 -g -Wall -I../include -I../speedo
LDFLAGS = 
LIBS= -lcurl -lpng12 -lz

SPDLIB = reset.o do_char.o do_trns.o set_spcs.o set_trns.o \
	out_bl2d.o out_blk.o out_scrn.o out_util.o out_outl.o \
	keys.o sp_err.o bics2uni.o

PROGRAMS = spdview.cgi

all: $(PROGRAMS)

spdview.cgi: spdview.o cgic.o glib.o $(SPDLIB) writepng.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	cp $@ ../www

spdview.o: spdview.c
	$(CC) $(CFLAGS) $< -c -o $@

cgic.o: cgic.c
	$(CC) $(CFLAGS) $< -c -o $@

glib.o: glib.c
	$(CC) $(CFLAGS) $< -c -o $@

reset.o: ../speedo/reset.c
	$(CC) $(CFLAGS) -c -o $@ $<

do_char.o: ../speedo/do_char.c
	$(CC) $(CFLAGS) -c -o $@ $<

do_trns.o: ../speedo/do_trns.c
	$(CC) $(CFLAGS) -c -o $@ $<

set_spcs.o: ../speedo/set_spcs.c
	$(CC) $(CFLAGS) -c -o $@ $<

set_trns.o: ../speedo/set_trns.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_bl2d.o: ../speedo/out_bl2d.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_blk.o: ../speedo/out_blk.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_scrn.o: ../speedo/out_scrn.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_outl.o: ../speedo/out_outl.c
	$(CC) $(CFLAGS) -c -o $@ $<

out_util.o: ../speedo/out_util.c
	$(CC) $(CFLAGS) -c -o $@ $<

keys.o: ../speedo/keys.c
	$(CC) $(CFLAGS) -c -o $@ $<

sp_err.o: ../speedo/sp_err.c
	$(CC) $(CFLAGS) -c -o $@ $<

bics2uni.o: ../speedo/bics2uni.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean::
	rm -f $(PROGRAMS) *.o
