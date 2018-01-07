#**************************************************************************
#*
#*  FreeType demo utilities sub-Makefile
#*
#*  This Makefile is to be included by `../Makefile'.  Its
#*  purpose is to compile MiGS (the Minimalist Graphics Subsystem).
#*
#*  It is written for GNU Make.  Other make utilities are not
#*  supported!
#*
#**************************************************************************


GRAPH_DIR = $(TOP_DIR_2)/graph

GRAPH_INCLUDES = -I$(GRAPH_DIR)
GRAPH_LIB      = $(GRAPH_DIR)/libgraph.a

GRAPH_H =  $(GRAPH_DIR)/gblany.h    \
           $(GRAPH_DIR)/gblbgra.h   \
           $(GRAPH_DIR)/gblblit.h   \
           $(GRAPH_DIR)/gblcolor.h  \
           $(GRAPH_DIR)/gblhbgr.h   \
           $(GRAPH_DIR)/gblhrgb.h   \
           $(GRAPH_DIR)/gblvbgr.h   \
           $(GRAPH_DIR)/gblvrgb.h   \
           $(GRAPH_DIR)/gblender.h  \
           $(GRAPH_DIR)/graph.h     \
           $(GRAPH_DIR)/grblit.h    \
           $(GRAPH_DIR)/grconfig.h  \
           $(GRAPH_DIR)/grdevice.h  \
           $(GRAPH_DIR)/grevents.h  \
           $(GRAPH_DIR)/grfont.h    \
           $(GRAPH_DIR)/grobjs.h    \
           $(GRAPH_DIR)/grswizzle.h \
           $(GRAPH_DIR)/grtypes.h


GRAPH_OBJS  = $(GRAPH_DIR)/gblblit.o   \
              $(GRAPH_DIR)/gblender.o  \
              $(GRAPH_DIR)/grblit.o    \
              $(GRAPH_DIR)/grdevice.o  \
              $(GRAPH_DIR)/grfill.o    \
              $(GRAPH_DIR)/grfont.o    \
              $(GRAPH_DIR)/grinit.o    \
              $(GRAPH_DIR)/grobjs.o    \
              $(GRAPH_DIR)/grswizzle.o



# Default value for COMPILE_GRAPH_LIB;
# this value can be modified by the system-specific graphics drivers.
#
COMPILE_GRAPH_LIB = $(AR) -r $@ $(GRAPH_OBJS)


# Add the rules used to detect and compile graphics driver depending
# on the current platform.

DEVICES =

ifdef OS2_SHELL
GRAPH_OBJS += $(GRAPH_DIR)/os2/gros2pm.o
GRAPH_LINK += $(GRAPH_DIR)/os2/gros2pm.def
GRAPH_INCLUDES += -I$(GRAPH_DIR)/os2
DEVICES += -DDEVICE_OS2_PM
endif

ifeq ($(OS),Windows_NT)
GRAPH_OBJS += $(GRAPH_DIR)/win32/grwin32.o
GRAPH_LINK += -luser32 -lgdi32
GRAPH_INCLUDES += -I$(GRAPH_DIR)/win32
DEVICES += -DDEVICE_WIN32
endif

ifeq ($(CROSS),win32)
GRAPH_OBJS += $(GRAPH_DIR)/win32/grwin32.o
GRAPH_LINK += -luser32 -lgdi32
GRAPH_INCLUDES += -I$(GRAPH_DIR)/win32
DEVICES += -DDEVICE_WIN32
endif

ifeq ($(shell uname -s),Darwin)
GRAPH_OBJS += $(GRAPH_DIR)/mac/grmac.o
GRAPH_LINK += 
GRAPH_INCLUDES += -I$(GRAPH_DIR)/mac
DEVICES += -DDEVICE_MAC
endif

ifeq ($(shell uname -s),BeOS)
GRAPH_OBJS += $(GRAPH_DIR)/beos/grbeos.o
GRAPH_LINK += 
GRAPH_INCLUDES += -I$(GRAPH_DIR)/beos
DEVICES += -DDEVICE_BEOS
endif

ifeq ($(DEVICES),)
GRAPH_OBJS += $(GRAPH_DIR)/grx11.o
GRAPH_LINK += -lX11
GRAPH_INCLUDES += -I$(GRAPH_DIR)/x11
DEVICES += -DDEVICE_X11
endif

# test for the `ALLEGRO' environment variable. This is non-optimal.
ifdef ALLEGRO
GRAPH_OBJS += $(GRAPH_DIR)/allegro/gralleg.o
GRAPH_LINK += -lalleg
GRAPH_INCLUDES += -I$(GRAPH_DIR)/allegro
DEVICES += -DDEVICE_ALLEGRO
endif


#########################################################################
#
# Build the `graph' library from its objects.  This should be changed
# in the future in order to support more systems.  Probably something
# like a `config/<system>' hierarchy with a system-specific rules file
# to indicate how to make a library file, but for now, I'll stick to
# unix, Win32, and OS/2-gcc.
#
#
$(GRAPH_LIB): $(GRAPH_OBJS)
	$(COMPILE_GRAPH_LIB)


# pattern rule for normal sources
#
$(GRAPH_DIR)/%.o: $(GRAPH_DIR)/%.c $(GRAPH_H)
	$(COMPILE) $(GRAPH_INCLUDES) $(DEVICES)

$(GRAPH_DIR)/grx11.o: $(GRAPH_DIR)/x11/grx11.c $(GRAPH_DIR)/x11/grx11.h
	$(COMPILE) $(GRAPH_INCLUDES) $(DEVICES)

$(GRAPH_DIR)/gralleg.o: $(GRAPH_DIR)/allegro/gralleg.c
	$(COMPILE) $(GRAPH_INCLUDES) $(DEVICES)

$(GRAPH_DIR)/gros2pm.o: $(GRAPH_DIR)/os2/gros2pm.c
	$(COMPILE) $(GRAPH_INCLUDES) $(DEVICES)

$(GRAPH_DIR)/grwin32.o: $(GRAPH_DIR)/win32/grwin32.c
	$(COMPILE) $(GRAPH_INCLUDES) $(DEVICES)

$(GRAPH_DIR)/grbeos.o: $(GRAPH_DIR)/beos/grbeos.c
	$(COMPILE) $(GRAPH_INCLUDES) $(DEVICES)

$(GRAPH_DIR)/grmac.o: $(GRAPH_DIR)/mac/grmac.c
	$(COMPILE) $(GRAPH_INCLUDES) $(DEVICES)
