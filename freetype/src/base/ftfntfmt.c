/***************************************************************************/
/*                                                                         */
/*  ftfntfmt.c                                                             */
/*                                                                         */
/*    FreeType utility file for font formats (body).                       */
/*                                                                         */
/*  Copyright 2002-2017 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2bld.h>
#include <ft2build.h>
#include <freetype/ftfntfmt.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/services/svfntfmt.h>

ANONYMOUS_STRUCT_DUMMY(FT_RasterRec_)
ANONYMOUS_STRUCT_DUMMY(FT_Size_InternalRec_)
ANONYMOUS_STRUCT_DUMMY(FT_IncrementalRec_)

  /* documentation is in ftfntfmt.h */

  FT_EXPORT_DEF( const char* )
  FT_Get_Font_Format( FT_Face  face )
  {
    const char*  result = NULL;


    if ( face )
      FT_FACE_FIND_SERVICE( face, result, FONT_FORMAT );

    return result;
  }


  /* deprecated function name; retained for ABI compatibility */

  FT_EXPORT_DEF( const char* )
  FT_Get_X11_Font_Format( FT_Face  face )
  {
    const char*  result = NULL;


    if ( face )
      FT_FACE_FIND_SERVICE( face, result, FONT_FORMAT );

    return result;
  }


/* END */