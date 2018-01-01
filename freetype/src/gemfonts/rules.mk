#
# FreeType 2 GEM FNT driver configuration rules
#


# Copyright 1996-2000, 2001, 2003 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


# GEM fonts driver directory
#
GEM_DIR := $(SRC_DIR)/gemfonts


GEM_COMPILE := $(FT_COMPILE) $I$(subst /,$(COMPILER_SEP),$(GEM_DIR))


# GEM driver sources (i.e., C files)
#
GEM_DRV_SRC := $(GEM_DIR)/gemfnt.c

# GEM fonts driver headers
#
GEM_DRV_H := $(GEM_DRV_SRC:%.c=%.h)


# GEM fonts driver object(s)
#
#   GEM_DRV_OBJ_M is used during `multi' builds
#   GEM_DRV_OBJ_S is used during `single' builds
#
GEM_DRV_OBJ_M := $(GEM_DRV_SRC:$(GEM_DIR)/%.c=$(OBJ_DIR)/%.$O)
GEM_DRV_OBJ_S := $(OBJ_DIR)/gemfnt.$O

# GEM fonts driver source file for single build
#
GEM_DRV_SRC_S := $(GEM_DIR)/gemfnt.c


# GEM fonts driver - single object
#
$(GEM_DRV_OBJ_S): $(GEM_DRV_SRC_S) $(GEM_DRV_SRC) $(FREETYPE_H) $(GEM_DRV_H)
	$(GEM_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $(GEM_DRV_SRC_S))


# GEM fonts driver - multiple objects
#
$(OBJ_DIR)/%.$O: $(GEM_DIR)/%.c $(FREETYPE_H) $(GEM_DRV_H)
	$(GEM_COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


# update main driver object lists
#
DRV_OBJS_S += $(GEM_DRV_OBJ_S)
DRV_OBJS_M += $(GEM_DRV_OBJ_M)


# EOF
