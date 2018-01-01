#
# FreeType 2 GEM FNT module definition
#


# Copyright 1996-2000, 2006 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


FTMODULE_H_COMMANDS += GEMFNT_DRIVER

define GEMFNT_DRIVER
$(OPEN_DRIVER) FT_Driver_ClassRec, gemfnt_driver_class $(CLOSE_DRIVER)
$(ECHO_DRIVER)gemfnt    $(ECHO_DRIVER_DESC)GEM bitmap fonts with extension *.fnt$(ECHO_DRIVER_DONE)
endef

# EOF
