/***************************************************************************/
/*                                                                         */
/*  sfnt.c                                                                 */
/*                                                                         */
/*    Single object library component.                                     */
/*                                                                         */
/*  Copyright 1996-2017 by                                                 */
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
ANONYMOUS_STRUCT_DUMMY(GX_BlendRec_)

#include "pngshim.c"
#include "sfdriver.c"
#include "sfobjs.c"
#include "ttbdf.c"
#include "ttcmap.c"
#include "ttkern.c"
#include "ttload.c"
#include "ttmtx.c"
#include "ttpost.c"
#include "ttsbit.c"
