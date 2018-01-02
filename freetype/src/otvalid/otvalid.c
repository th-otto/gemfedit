/***************************************************************************/
/*                                                                         */
/*  otvalid.c                                                              */
/*                                                                         */
/*    FreeType validator for OpenType tables (body only).                  */
/*                                                                         */
/*  Copyright 2004-2017 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#define FT_MAKE_OPTION_SINGLE_OBJECT
#include <ft2bld.h>
#include <ft2build.h>

ANONYMOUS_STRUCT_DUMMY(FT_RasterRec_)
ANONYMOUS_STRUCT_DUMMY(FT_IncrementalRec_)

#include "otvbase.c"
#include "otvcommn.c"
#include "otvgdef.c"
#include "otvgpos.c"
#include "otvgsub.c"
#include "otvjstf.c"
#include "otvmath.c"
#include "otvmod.c"


/* END */
