VERSION = 1.0.0

SUBDIRS = fonts include tools tos unix win32

EXTRA_DIST = Makefile

all clean::
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@; done

dist::
	$(MAKE) clean
	$(MAKE) -C tos
	rm -f gemfedit-$(VERSION)-tos.zip
	zip -j gemfedit-$(VERSION)-tos.zip tos/*.prg tos/*.rsc
	$(MAKE) -C win32
	rm -f fontdisp-$(VERSION)-win32.zip
	zip -j fontdisp-$(VERSION)-win32.zip win32/*.exe
	$(MAKE) -C unix
	rm -f fontdisp-$(VERSION)-unix.zip
	zip -j fontdisp-$(VERSION)-unix.zip unix/fontdisp
	$(MAKE) clean
	rm -f gemfedit-$(VERSION)-src.zip
	zip -r gemfedit-$(VERSION)-src.zip $(EXTRA_DIST) $(SUBDIRS)
