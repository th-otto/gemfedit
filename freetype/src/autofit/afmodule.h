/***************************************************************************/
/*                                                                         */
/*  afmodule.h                                                             */
/*                                                                         */
/*    Auto-fitter module implementation (specification).                   */
/*                                                                         */
/*  Copyright 2003-2017 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef AFMODULE_H_
#define AFMODULE_H_

#include <ft2build.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/ftmodapi.h>


FT_BEGIN_HEADER

/*
 *  This is the `extended' FT_Module structure that holds the
 *  autofitter's global data.
 */
typedef struct AF_ModuleRec_
{
	FT_ModuleRec root;

	FT_UInt fallback_style;
	FT_UInt default_script;
#ifdef AF_CONFIG_OPTION_USE_WARPER
	FT_Bool warping;
#endif
	FT_Bool no_stem_darkening;
	FT_Int darken_params[8];

} AF_ModuleRec, *AF_Module;


FT_DECLARE_MODULE(autofit_module_class)

#ifdef FT_DEBUG_AUTOFIT
void af_glyph_hints_dump_segments(AF_GlyphHints hints, FT_Bool to_stdout);
void af_glyph_hints_dump_points(AF_GlyphHints hints, FT_Bool to_stdout);
void af_glyph_hints_dump_edges(AF_GlyphHints hints, FT_Bool to_stdout);
FT_Error af_glyph_hints_get_num_segments(AF_GlyphHints hints, FT_Int dimension, FT_Int *num_segments);
FT_Error af_glyph_hints_get_segment_offset(AF_GlyphHints hints, FT_Int dimension, FT_Int idx, FT_Pos *offset, FT_Bool *is_blue, FT_Pos *blue_offset);
#endif

FT_END_HEADER

#endif /* AFMODULE_H_ */
