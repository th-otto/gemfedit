CC = gcc
AR = ar
RANLIB = ranlib

ft2_srcdir   = ..
TOP_DIR_2 = .
BIN_DIR_2 = $(TOP_DIR_2)/bin
OBJ_DIR_2 = .

WARN = -fno-common -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes
CFLAGS = -O2 -g -fomit-frame-pointer $(WARN) -I$(ft2_srcdir)/../include -I$(ft2_srcdir)/include
LDFLAGS = -s
LIBS= -lm -lrt -lz -lbz2 `pkg-config --libs libpng harfbuzz`

all: exes


####################################################################
#
# ft2_srcdir is the directory where the main FreeType source is found.
#
# TOP_DIR_2 is the directory is the top of the demonstration
# programs directory.
#
# OBJ_DIR gives the objects directory of the FreeType library.
#

GRAPH_DIR = $(TOP_DIR_2)/graph

SRC_DIR = $(TOP_DIR_2)/src

COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) $(ANSIFLAGS) -c -o $@ $<

FTLIB = $(ft2_srcdir)/libttf.a
GRAPH_LIB = $(GRAPH_DIR)/libgraph.a

# The default variables used to link the executables.  These can
# be redefined for platform-specific stuff.
#
# The first token of LINK_ITEMS must be the executable.
#
LINK_CMD = $(CC) $(LDFLAGS)
LINK_ITEMS = -o $@ $<
LINK_LIBS = -L$(ft2_srcdir) -lttf $(LIBS)
GRAPH_INCLUDES = -I$(GRAPH_DIR)

LINK        = $(LINK_CMD) $(LINK_ITEMS) $(LINK_LIBS)
LINK_COMMON = $(LINK_CMD) $(LINK_ITEMS) $(COMMON_OBJ) $(LINK_LIBS)
LINK_GRAPH  = $(LINK_COMMON) $(GRAPH_LIB) $(GRAPH_LINK) $(MATH)
LINK_NEW    = $(LINK_CMD) $(LINK_ITEMS) $(COMMON_OBJ) $(FTCOMMON_OBJ) $(LINK_LIBS) $(GRAPH_LIB) $(GRAPH_LINK) $(MATH)

.PHONY: exes clean distclean


clean::
	    -$(RM) $(OBJ_DIR_2)/*.o $(OBJ_DIR_2)/*.a $(GRAPH_DIR)/*.o $(GRAPH_DIR)/*.a $(EXES)

distclean:: clean


####################################################################
#
# POSIX TERMIOS: Do not define if you use OLD U*ix like 4.2BSD.
#
EXTRAFLAGS = -DUNIX -DHAVE_POSIX_TERMIOS


###################################################################
#
# The list of demonstration programs to build.
#
# Note that ttdebug only works if the FreeType's `truetype' driver has
# been compiled with TT_CONFIG_OPTION_BYTECODE_INTERPRETER defined.
#
EXES =  $(BIN_DIR_2)/ftbench$(EXEEXT) \
	$(BIN_DIR_2)/ftdump$(EXEEXT)  \
	$(BIN_DIR_2)/ftlint$(EXEEXT)  \
	$(BIN_DIR_2)/ttdebug$(EXEEXT)

# Comment out the next line if you don't have a graphics subsystem.
EXES += $(BIN_DIR_2)/ftdiff$(EXEEXT)   \
	$(BIN_DIR_2)/ftgamma$(EXEEXT)  \
	$(BIN_DIR_2)/ftgrid$(EXEEXT)   \
	$(BIN_DIR_2)/ftmulti$(EXEEXT)  \
	$(BIN_DIR_2)/ftstring$(EXEEXT) \
	$(BIN_DIR_2)/ftview$(EXEEXT) \
	$(BIN_DIR_2)/ftvalid$(EXEEXT) \
	$(BIN_DIR_2)/xtest$(EXEEXT)

# The following programs are not compiled automatically; either comment
# out the affected line or use the program name as a Makefile target.
#
# EXES += $(BIN_DIR_2)/ftchkwd$(EXEEXT)
# EXES += $(BIN_DIR_2)/ftmemchk$(EXEEXT)
# EXES += $(BIN_DIR_2)/ftpatchk$(EXEEXT)
# EXES += $(BIN_DIR_2)/fttimer$(EXEEXT)
# EXES += $(BIN_DIR_2)/testname$(EXEEXT)

exes: $(EXES)


# generic rule
$(OBJ_DIR_2)/%.o: $(SRC_DIR)/%.c $(FTLIB)
	  $(COMPILE)

include $(GRAPH_DIR)/rules.mk

####################################################################
#
# Rules for compiling object files for text-only demos.
#
$(OBJ_DIR_2)/common.o: $(SRC_DIR)/common.c
$(OBJ_DIR_2)/output.o: $(SRC_DIR)/output.c
$(OBJ_DIR_2)/mlgetopt.o: $(SRC_DIR)/mlgetopt.c
COMMON_OBJ =  $(OBJ_DIR_2)/common.o $(OBJ_DIR_2)/output.o $(OBJ_DIR_2)/mlgetopt.o


FTCOMMON_OBJ = $(OBJ_DIR_2)/ftcommon.o
$(FTCOMMON_OBJ): $(SRC_DIR)/ftcommon.c $(SRC_DIR)/ftcommon.h
	  $(COMPILE) $(GRAPH_INCLUDES)


$(OBJ_DIR_2)/ftlint.o: $(SRC_DIR)/ftlint.c
	  $(COMPILE)

$(OBJ_DIR_2)/ftbench.o: $(SRC_DIR)/ftbench.c
	  $(COMPILE) $(EXTRAFLAGS)

$(OBJ_DIR_2)/ftpatchk.o: $(SRC_DIR)/ftpatchk.c
	  $(COMPILE) $(EXTRAFLAGS)

$(OBJ_DIR_2)/ftchkwd.o: $(SRC_DIR)/ftchkwd.c
	  $(COMPILE) $(EXTRAFLAGS)

$(OBJ_DIR_2)/compos.o: $(SRC_DIR)/compos.c
	  $(COMPILE)

$(OBJ_DIR_2)/ftmemchk.o: $(SRC_DIR)/ftmemchk.c
	  $(COMPILE)

$(OBJ_DIR_2)/fttimer.o: $(SRC_DIR)/fttimer.c
	  $(COMPILE)

$(OBJ_DIR_2)/fttry.o: $(SRC_DIR)/fttry.c
	  $(COMPILE)

$(OBJ_DIR_2)/testname.o: $(SRC_DIR)/testname.c
	  $(COMPILE)

#  $(OBJ_DIR_2)/ftsbit.o: $(SRC_DIR)/ftsbit.c
#	  $(COMPILE)

$(OBJ_DIR_2)/xtest.o: graph/xtest.c
	  $(COMPILE)


# We simplify the dependencies on the graphics library by using
# $(GRAPH_LIB) directly.

$(OBJ_DIR_2)/ftgamma.o: $(SRC_DIR)/ftgamma.c $(SRC_DIR)/ftcommon.h $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES)

$(OBJ_DIR_2)/ftmulti.o: $(SRC_DIR)/ftmulti.c $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES)

$(OBJ_DIR_2)/ftstring.o: $(SRC_DIR)/ftstring.c $(SRC_DIR)/ftcommon.h $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES)


####################################################################
#
# Special rule to compile the `ftdump' program as it includes
# internal header files.
#
$(OBJ_DIR_2)/ftdump.o: $(SRC_DIR)/ftdump.c
	  $(COMPILE) -DFT2_BUILD_LIBRARY


####################################################################
#
# Special rule to compile the `t1dump' program as it includes
# the Type1 source path.
#
$(OBJ_DIR_2)/t1dump.o: $(SRC_DIR)/t1dump.c
	  $(COMPILE)


####################################################################
#
# Special rule to compile the `ftvalid' program as it includes
# internal header files.
#
$(OBJ_DIR_2)/ftvalid.o: $(SRC_DIR)/ftvalid.c
	  $(COMPILE) -DFT2_BUILD_LIBRARY


####################################################################
#
# Special rule to compile the `ftview' program as it includes
# internal header files.
#
$(OBJ_DIR_2)/ftview.o: $(SRC_DIR)/ftview.c $(SRC_DIR)/ftcommon.h $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES) -DFT2_BUILD_LIBRARY


####################################################################
#
# Special rule to compile the `ftdiff' program as it includes
# internal header files.
#
$(OBJ_DIR_2)/ftdiff.o: $(SRC_DIR)/ftdiff.c $(SRC_DIR)/ftcommon.h $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES) -DFT2_BUILD_LIBRARY


####################################################################
#
# Special rule to compile the `ftgrid' program as it includes
# internal header files.
#
$(OBJ_DIR_2)/ftgrid.o: $(SRC_DIR)/ftgrid.c $(SRC_DIR)/ftcommon.h $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) -DFT2_BUILD_LIBRARY


####################################################################
#
# Special rule to compile the `ttdebug' program as it includes
# the TrueType source path and needs extra flags for correct keyboard
# handling on Unix.

$(OBJ_DIR_2)/ttdebug.o: $(SRC_DIR)/ttdebug.c
	  $(COMPILE) -I$(ft2_srcdir)/src/truetype $(EXTRAFLAGS) -DFT2_BUILD_LIBRARY


####################################################################
#
# Rules used to link the executables.  Note that they could be
# overridden by system-specific things.
#
$(BIN_DIR_2)/ftlint$(EXEEXT): $(OBJ_DIR_2)/ftlint.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/ftbench$(EXEEXT): $(OBJ_DIR_2)/ftbench.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/ftpatchk$(EXEEXT): $(OBJ_DIR_2)/ftpatchk.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/ftchkwd$(EXEEXT): $(OBJ_DIR_2)/ftchkwd.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/ftmemchk$(EXEEXT): $(OBJ_DIR_2)/ftmemchk.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/compos$(EXEEXT): $(OBJ_DIR_2)/compos.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/ftvalid$(EXEEXT): $(OBJ_DIR_2)/ftvalid.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/ftdump$(EXEEXT): $(OBJ_DIR_2)/ftdump.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/fttimer$(EXEEXT): $(OBJ_DIR_2)/fttimer.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/fttry$(EXEEXT): $(OBJ_DIR_2)/fttry.o $(FTLIB)
	  $(LINK)

#  $(BIN_DIR_2)/ftsbit$(EXEEXT): $(OBJ_DIR_2)/ftsbit.o $(FTLIB)
#	  $(LINK)

$(BIN_DIR_2)/t1dump$(EXEEXT): $(OBJ_DIR_2)/t1dump.o $(FTLIB)
	  $(LINK)

$(BIN_DIR_2)/ttdebug$(EXEEXT): $(OBJ_DIR_2)/ttdebug.o $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

$(BIN_DIR_2)/testname$(EXEEXT): $(OBJ_DIR_2)/testname.o $(FTLIB)
	  $(LINK)


$(BIN_DIR_2)/ftview$(EXEEXT): $(OBJ_DIR_2)/ftview.o $(FTLIB) $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

$(BIN_DIR_2)/ftgrid$(EXEEXT): $(OBJ_DIR_2)/ftgrid.o $(FTLIB) $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

$(BIN_DIR_2)/ftgamma$(EXEEXT): $(OBJ_DIR_2)/ftgamma.o $(FTLIB) $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

$(BIN_DIR_2)/ftdiff$(EXEEXT): $(OBJ_DIR_2)/ftdiff.o $(FTLIB) $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

$(BIN_DIR_2)/ftmulti$(EXEEXT): $(OBJ_DIR_2)/ftmulti.o $(FTLIB) $(GRAPH_LIB) $(COMMON_OBJ)
	  $(LINK_GRAPH)

$(BIN_DIR_2)/ftstring$(EXEEXT): $(OBJ_DIR_2)/ftstring.o $(FTLIB) $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

$(BIN_DIR_2)/xtest$(EXEEXT): $(OBJ_DIR_2)/xtest.o $(FTLIB) $(GRAPH_LIB) $(COMMON_OBJ)
	  $(LINK_GRAPH)
