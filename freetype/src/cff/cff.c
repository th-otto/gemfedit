/***************************************************************************/
/*                                                                         */
/*  cff.c                                                                  */
/*                                                                         */
/*    FreeType OpenType driver component (body only).                      */
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
ANONYMOUS_STRUCT_DUMMY(T1_HintsRec_)
ANONYMOUS_STRUCT_DUMMY(T2_HintsRec_)
ANONYMOUS_STRUCT_DUMMY(PSH_GlobalsRec_)

#include "cffcmap.c"
#include "cffdrivr.c"
#include "cffgload.c"
#include "cffload.c"
#include "cffobjs.c"
#include "cffparse.c"
#include "cffpic.c"
#include "cf2arrst.c"
#include "cf2blues.c"
#include "cf2error.c"
#include "cf2font.c"
#include "cf2ft.c"
#include "cf2hints.c"
#include "cf2intrp.c"
#include "cf2read.c"
#include "cf2stack.c"
