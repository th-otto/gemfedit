ifeq ($(CROSS),win32)
CROSS_PREFIX=i686-pc-mingw32-
endif

ifeq ($(CROSS),mint)
CROSS_PREFIX=m68k-atari-mint-
endif

CC = $(CROSS_PREFIX)gcc
AR = $(CROSS_PREFIX)ar
RANLIB = $(CROSS_PREFIX)ranlib

ft2_srcdir = .
WARN = -fno-common -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes
CFLAGS = -O2 -fomit-frame-pointer $(WARN) -I../include -I$(ft2_srcdir)/include
LDFLAGS = -s
LIBS= -lX11

PROGRAMS = libttf.a

all: $(PROGRAMS)

ftdebug.o: $(ft2_srcdir)/src/base/ftdebug.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftbase.o: $(ft2_srcdir)/src/base/ftbase.c \
		$(ft2_srcdir)/src/base/ftadvanc.c \
		$(ft2_srcdir)/src/base/ftcalc.c \
		$(ft2_srcdir)/src/base/ftdbgmem.c \
		$(ft2_srcdir)/src/base/ftfntfmt.c \
		$(ft2_srcdir)/src/base/ftgloadr.c \
		$(ft2_srcdir)/src/base/fthash.c \
		$(ft2_srcdir)/src/base/ftmac.c \
		$(ft2_srcdir)/src/base/ftobjs.c \
		$(ft2_srcdir)/src/base/ftoutln.c \
		$(ft2_srcdir)/src/base/ftrfork.c \
		$(ft2_srcdir)/src/base/ftsnames.c \
		$(ft2_srcdir)/src/base/ftstream.c \
		$(ft2_srcdir)/src/base/fttrigon.c \
		$(ft2_srcdir)/src/base/ftutil.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftbbox.o: $(ft2_srcdir)/src/base/ftbbox.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftbdf.o: $(ft2_srcdir)/src/base/ftbdf.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftbitmap.o: $(ft2_srcdir)/src/base/ftbitmap.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftcid.o: $(ft2_srcdir)/src/base/ftcid.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

fterrors.o: $(ft2_srcdir)/src/base/fterrors.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftfstype.o: $(ft2_srcdir)/src/base/ftfstype.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftgasp.o: $(ft2_srcdir)/src/base/ftgasp.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftgemfnt.o: $(ft2_srcdir)/src/base/ftgemfnt.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftglyph.o: $(ft2_srcdir)/src/base/ftglyph.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftgxval.o: $(ft2_srcdir)/src/base/ftgxval.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftinit.o: $(ft2_srcdir)/src/base/ftinit.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftlcdfil.o: $(ft2_srcdir)/src/base/ftlcdfil.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftmm.o: $(ft2_srcdir)/src/base/ftmm.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftotval.o: $(ft2_srcdir)/src/base/ftotval.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftpatent.o: $(ft2_srcdir)/src/base/ftpatent.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftpfr.o: $(ft2_srcdir)/src/base/ftpfr.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftstroke.o: $(ft2_srcdir)/src/base/ftstroke.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftsynth.o: $(ft2_srcdir)/src/base/ftsynth.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftsystem.o: $(ft2_srcdir)/src/base/ftsystem.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

fttype1.o: $(ft2_srcdir)/src/base/fttype1.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftwinfnt.o: $(ft2_srcdir)/src/base/ftwinfnt.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<



autofit.o: $(ft2_srcdir)/src/autofit/autofit.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

bdf.o: $(ft2_srcdir)/src/bdf/bdf.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftbzip2.o: $(ft2_srcdir)/src/bzip2/ftbzip2.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftcache.o: $(ft2_srcdir)/src/cache/ftcache.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

cff.o: $(ft2_srcdir)/src/cff/cff.c \
	$(ft2_srcdir)/src/cff/cffcmap.c \
	$(ft2_srcdir)/src/cff/cffdrivr.c \
	$(ft2_srcdir)/src/cff/cffgload.c \
	$(ft2_srcdir)/src/cff/cffload.c \
	$(ft2_srcdir)/src/cff/cffobjs.c \
	$(ft2_srcdir)/src/cff/cffparse.c \
	$(ft2_srcdir)/src/cff/cf2arrst.c \
	$(ft2_srcdir)/src/cff/cf2blues.c \
	$(ft2_srcdir)/src/cff/cf2error.c \
	$(ft2_srcdir)/src/cff/cf2font.c \
	$(ft2_srcdir)/src/cff/cf2ft.c \
	$(ft2_srcdir)/src/cff/cf2hints.c \
	$(ft2_srcdir)/src/cff/cf2intrp.c \
	$(ft2_srcdir)/src/cff/cf2read.c \
	$(ft2_srcdir)/src/cff/cf2stack.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

type1cid.o: $(ft2_srcdir)/src/cid/type1cid.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftgzip.o: $(ft2_srcdir)/src/gzip/ftgzip.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftlzw.o: $(ft2_srcdir)/src/lzw/ftlzw.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

pcf.o: $(ft2_srcdir)/src/pcf/pcf.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

pfr.o: $(ft2_srcdir)/src/pfr/pfr.c \
	$(ft2_srcdir)/src/pfr/pfrcmap.c \
	$(ft2_srcdir)/src/pfr/pfrdrivr.c \
	$(ft2_srcdir)/src/pfr/pfrgload.c \
	$(ft2_srcdir)/src/pfr/pfrload.c \
	$(ft2_srcdir)/src/pfr/pfrobjs.c \
	$(ft2_srcdir)/src/pfr/pfrsbit.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

psaux.o: $(ft2_srcdir)/src/psaux/psaux.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

pshinter.o: $(ft2_srcdir)/src/pshinter/pshinter.c \
	$(ft2_srcdir)/src/pshinter/pshalgo.c \
	$(ft2_srcdir)/src/pshinter/pshglob.c \
	$(ft2_srcdir)/src/pshinter/pshmod.c \
	$(ft2_srcdir)/src/pshinter/pshrec.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

psnames.o: $(ft2_srcdir)/src/psnames/psnames.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

raster.o: $(ft2_srcdir)/src/raster/raster.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

sfnt.o: $(ft2_srcdir)/src/sfnt/sfnt.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

smooth.o: $(ft2_srcdir)/src/smooth/smooth.c \
	$(ft2_srcdir)/src/smooth/ftgrays.c \
	$(ft2_srcdir)/src/smooth/ftsmooth.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

truetype.o: $(ft2_srcdir)/src/truetype/truetype.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

type1.o: $(ft2_srcdir)/src/type1/type1.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

type42.o: $(ft2_srcdir)/src/type42/type42.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

winfnt.o: $(ft2_srcdir)/src/winfonts/winfnt.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

gemfnt.o: $(ft2_srcdir)/src/gemfonts/gemfnt.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<


otvalid.o: $(ft2_srcdir)/src/otvalid/otvalid.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

gxvalid.o: $(ft2_srcdir)/src/gxvalid/gxvalid.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<




BASEOBJ = \
	ftdebug.o ftbase.o ftbbox.o ftbdf.o ftbitmap.o ftcid.o fterrors.o \
	ftfstype.o ftgasp.o ftgemfnt.o ftglyph.o ftgxval.o ftinit.o \
	ftlcdfil.o ftmm.o ftotval.o ftpatent.o ftpfr.o ftstroke.o \
	ftsynth.o ftsystem.o fttype1.o

AHINTOBJ = autofit.o

PSOBJ = psaux.o pshinter.o psnames.o

RASTEROBJ = raster.o smooth.o

GZOBJ = ftgzip.o ftbzip2.o ftlzw.o

# pcf.o  - needs GZLib and ZLib
PCFOBJ = pcf.o 
FONTDOBJ = cff.o type1.o type42.o type1cid.o truetype.o bdf.o pfr.o winfnt.o gemfnt.o $(PCFOBJ)

TTFLIB = $(BASEOBJ) $(AHINTOBJ) $(PSOBJ) $(RASTEROBJ) sfnt.o ftcache.o otvalid.o gxvalid.o $(FONTDOBJ) $(GZOBJ)

libttf.a: $(TTFLIB)
	$(AM_V_AR)$(AR) cr $@ $(TTFLIB); $(RANLIB) $@

clean::
	$(RM) $(TTFLIB) $(PROGRAMS)

install::
	@:
