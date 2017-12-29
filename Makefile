VERSION = 1.0.0

SUBDIRS = fonts include tools tos unix win32 speedo spdview

EXTRA_DIST = Makefile README COPYING

all clean::
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@; done

dist::
	: $(MAKE) clean
	: $(MAKE) -C tos
	$(MAKE) -C win32
	$(MAKE) -C unix
	rm -f */*.o
	rm -f gemfedit-$(VERSION)-src.zip
	zip -r gemfedit-$(VERSION)-src.zip $(EXTRA_DIST) $(SUBDIRS)
