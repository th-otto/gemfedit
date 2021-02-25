VERSION = 1.0.3

SUBDIRS = fonts include freetype tools tos unix win32 speedo spdview

EXTRA_DIST = Makefile README COPYING

all clean::
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@; done

dist::
	$(MAKE) clean
	$(MAKE) -C freetype clean
	$(MAKE) -C freetype/ft2demos clean
	$(MAKE) -C freetype/ft2demos/src/ftinspect clean
	: $(MAKE) -C tos
	$(MAKE) -C win32
	$(MAKE) -C unix
	cp -a /windows/c/atari/src/gemfedit/tos/gemfedit.prg tos
	cp -a /windows/c/atari/src/gemfedit/tos/spdview.prg tos
	cp -a /windows/c/atari/src/gemfedit/tos/fontdisp.prg tos
	rm -f */*.o
	rm -f gemfedit-$(VERSION)-src.zip
	zip -r gemfedit-$(VERSION)-src.zip $(EXTRA_DIST) $(SUBDIRS)
