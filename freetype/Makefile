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
CFLAGS = -O2 -fomit-frame-pointer $(WARN) -I../include -I$(ft2_srcdir)/include -I../libpng -I../zlib
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



autofit.o: $(ft2_srcdir)/src/autofit/autofit.c \
	$(ft2_srcdir)/src/autofit/afangles.c \
	$(ft2_srcdir)/src/autofit/afblue.c \
	$(ft2_srcdir)/src/autofit/afcjk.c \
	$(ft2_srcdir)/src/autofit/afdummy.c \
	$(ft2_srcdir)/src/autofit/afglobal.c \
	$(ft2_srcdir)/src/autofit/afhints.c \
	$(ft2_srcdir)/src/autofit/afindic.c \
	$(ft2_srcdir)/src/autofit/aflatin.c \
	$(ft2_srcdir)/src/autofit/aflatin2.c \
	$(ft2_srcdir)/src/autofit/afloader.c \
	$(ft2_srcdir)/src/autofit/afmodule.c \
	$(ft2_srcdir)/src/autofit/afranges.c \
	$(ft2_srcdir)/src/autofit/afshaper.c \
	$(ft2_srcdir)/src/autofit/afwarp.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

bdf.o: $(ft2_srcdir)/src/bdf/bdf.c \
	$(ft2_srcdir)/src/bdf/bdflib.c \
	$(ft2_srcdir)/src/bdf/bdfdrivr.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftbzip2.o: $(ft2_srcdir)/src/bzip2/ftbzip2.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftcache.o: $(ft2_srcdir)/src/cache/ftcache.c \
	$(ft2_srcdir)/src/cache/ftcbasic.c \
	$(ft2_srcdir)/src/cache/ftccache.c \
	$(ft2_srcdir)/src/cache/ftccmap.c \
	$(ft2_srcdir)/src/cache/ftcglyph.c \
	$(ft2_srcdir)/src/cache/ftcimage.c \
	$(ft2_srcdir)/src/cache/ftcmanag.c \
	$(ft2_srcdir)/src/cache/ftcmru.c \
	$(ft2_srcdir)/src/cache/ftcsbits.c
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

type1cid.o: $(ft2_srcdir)/src/cid/type1cid.c \
	$(ft2_srcdir)/src/cid/cidgload.c \
	$(ft2_srcdir)/src/cid/cidload.c \
	$(ft2_srcdir)/src/cid/cidobjs.c \
	$(ft2_srcdir)/src/cid/cidparse.c \
	$(ft2_srcdir)/src/cid/cidriver.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftgzip.o: $(ft2_srcdir)/src/gzip/ftgzip.c \
	$(ft2_srcdir)/src/gzip/zutil.c \
	$(ft2_srcdir)/src/gzip/inftrees.c \
	$(ft2_srcdir)/src/gzip/infutil.c \
	$(ft2_srcdir)/src/gzip/infcodes.c \
	$(ft2_srcdir)/src/gzip/infblock.c \
	$(ft2_srcdir)/src/gzip/inflate.c \
	$(ft2_srcdir)/src/gzip/adler32.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

ftlzw.o: $(ft2_srcdir)/src/lzw/ftlzw.c \
	$(ft2_srcdir)/src/lzw/ftzopen.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

pcf.o: $(ft2_srcdir)/src/pcf/pcf.c \
	$(ft2_srcdir)/src/pcf/pcfdrivr.c \
	$(ft2_srcdir)/src/pcf/pcfread.c \
	$(ft2_srcdir)/src/pcf/pcfutil.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

pfr.o: $(ft2_srcdir)/src/pfr/pfr.c \
	$(ft2_srcdir)/src/pfr/pfrcmap.c \
	$(ft2_srcdir)/src/pfr/pfrdrivr.c \
	$(ft2_srcdir)/src/pfr/pfrgload.c \
	$(ft2_srcdir)/src/pfr/pfrload.c \
	$(ft2_srcdir)/src/pfr/pfrobjs.c \
	$(ft2_srcdir)/src/pfr/pfrsbit.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

psaux.o: $(ft2_srcdir)/src/psaux/psaux.c \
	$(ft2_srcdir)/src/psaux/afmparse.c \
	$(ft2_srcdir)/src/psaux/psauxmod.c \
	$(ft2_srcdir)/src/psaux/psconv.c \
	$(ft2_srcdir)/src/psaux/psobjs.c \
	$(ft2_srcdir)/src/psaux/t1cmap.c \
	$(ft2_srcdir)/src/psaux/t1decode.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

pshinter.o: $(ft2_srcdir)/src/pshinter/pshinter.c \
	$(ft2_srcdir)/src/pshinter/pshalgo.c \
	$(ft2_srcdir)/src/pshinter/pshglob.c \
	$(ft2_srcdir)/src/pshinter/pshmod.c \
	$(ft2_srcdir)/src/pshinter/pshrec.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

psnames.o: $(ft2_srcdir)/src/psnames/psnames.c \
	$(ft2_srcdir)/src/psnames/psmodule.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

raster.o: $(ft2_srcdir)/src/raster/raster.c \
	$(ft2_srcdir)/src/raster/ftraster.c \
	$(ft2_srcdir)/src/raster/ftrend1.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

sfnt.o: $(ft2_srcdir)/src/sfnt/sfnt.c \
	$(ft2_srcdir)/src/sfnt/pngshim.c \
	$(ft2_srcdir)/src/sfnt/sfdriver.c \
	$(ft2_srcdir)/src/sfnt/sfobjs.c \
	$(ft2_srcdir)/src/sfnt/ttbdf.c \
	$(ft2_srcdir)/src/sfnt/ttcmap.c \
	$(ft2_srcdir)/src/sfnt/ttkern.c \
	$(ft2_srcdir)/src/sfnt/ttload.c \
	$(ft2_srcdir)/src/sfnt/ttmtx.c \
	$(ft2_srcdir)/src/sfnt/ttpost.c \
	$(ft2_srcdir)/src/sfnt/ttsbit.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

smooth.o: $(ft2_srcdir)/src/smooth/smooth.c \
	$(ft2_srcdir)/src/smooth/ftgrays.c \
	$(ft2_srcdir)/src/smooth/ftsmooth.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

truetype.o: $(ft2_srcdir)/src/truetype/truetype.c \
	$(ft2_srcdir)/src/truetype/ttdriver.c \
	$(ft2_srcdir)/src/truetype/ttgload.c \
	$(ft2_srcdir)/src/truetype/ttgxvar.c \
	$(ft2_srcdir)/src/truetype/ttinterp.c \
	$(ft2_srcdir)/src/truetype/ttobjs.c \
	$(ft2_srcdir)/src/truetype/ttpload.c \
	$(ft2_srcdir)/src/truetype/ttsubpix.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

type1.o: $(ft2_srcdir)/src/type1/type1.c \
	$(ft2_srcdir)/src/type1/t1afm.c \
	$(ft2_srcdir)/src/type1/t1driver.c \
	$(ft2_srcdir)/src/type1/t1gload.c \
	$(ft2_srcdir)/src/type1/t1load.c \
	$(ft2_srcdir)/src/type1/t1objs.c \
	$(ft2_srcdir)/src/type1/t1parse.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

type42.o: $(ft2_srcdir)/src/type42/type42.c \
	$(ft2_srcdir)/src/type42/t42drivr.c \
	$(ft2_srcdir)/src/type42/t42objs.c \
	$(ft2_srcdir)/src/type42/t42parse.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

winfnt.o: $(ft2_srcdir)/src/winfonts/winfnt.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

gemfnt.o: $(ft2_srcdir)/src/gemfonts/gemfnt.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<


otvalid.o: $(ft2_srcdir)/src/otvalid/otvalid.c \
	$(ft2_srcdir)/src/otvalid/otvbase.c \
	$(ft2_srcdir)/src/otvalid/otvcommn.c \
	$(ft2_srcdir)/src/otvalid/otvgdef.c \
	$(ft2_srcdir)/src/otvalid/otvgpos.c \
	$(ft2_srcdir)/src/otvalid/otvgsub.c \
	$(ft2_srcdir)/src/otvalid/otvjstf.c \
	$(ft2_srcdir)/src/otvalid/otvmath.c \
	$(ft2_srcdir)/src/otvalid/otvmod.c
	$(AM_V_CC)$(CC) -c $(CFLAGS) -o $@ $<

gxvalid.o: $(ft2_srcdir)/src/gxvalid/gxvalid.c \
	$(ft2_srcdir)/src/gxvalid/gxvbsln.c \
	$(ft2_srcdir)/src/gxvalid/gxvcommn.c \
	$(ft2_srcdir)/src/gxvalid/gxvfeat.c \
	$(ft2_srcdir)/src/gxvalid/gxvjust.c \
	$(ft2_srcdir)/src/gxvalid/gxvkern.c \
	$(ft2_srcdir)/src/gxvalid/gxvlcar.c \
	$(ft2_srcdir)/src/gxvalid/gxvmod.c \
	$(ft2_srcdir)/src/gxvalid/gxvmort.c \
	$(ft2_srcdir)/src/gxvalid/gxvmort0.c \
	$(ft2_srcdir)/src/gxvalid/gxvmort1.c \
	$(ft2_srcdir)/src/gxvalid/gxvmort2.c \
	$(ft2_srcdir)/src/gxvalid/gxvmort4.c \
	$(ft2_srcdir)/src/gxvalid/gxvmort5.c \
	$(ft2_srcdir)/src/gxvalid/gxvmorx.c \
	$(ft2_srcdir)/src/gxvalid/gxvmorx0.c \
	$(ft2_srcdir)/src/gxvalid/gxvmorx1.c \
	$(ft2_srcdir)/src/gxvalid/gxvmorx2.c \
	$(ft2_srcdir)/src/gxvalid/gxvmorx4.c \
	$(ft2_srcdir)/src/gxvalid/gxvmorx5.c \
	$(ft2_srcdir)/src/gxvalid/gxvopbd.c \
	$(ft2_srcdir)/src/gxvalid/gxvprop.c \
	$(ft2_srcdir)/src/gxvalid/gxvtrak.c
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
	@rm -f $@
	$(AM_V_AR)$(AR) crs $@ $(TTFLIB); $(RANLIB) $@

clean::
	$(RM) $(TTFLIB) $(PROGRAMS)

install::
	@:
