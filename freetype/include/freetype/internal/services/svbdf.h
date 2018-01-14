/***************************************************************************/
/*                                                                         */
/*  svbdf.h                                                                */
/*                                                                         */
/*    The FreeType BDF services (specification).                           */
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


#ifndef SVBDF_H_
#define SVBDF_H_

#include <freetype/ftbdf.h>
#include <freetype/internal/ftserv.h>


FT_BEGIN_HEADER

#define FT_SERVICE_ID_BDF  "bdf"
typedef FT_Error (*FT_BDF_GetCharsetIdFunc) (FT_Face face, const char * *acharset_encoding, const char * *acharset_registry);

typedef FT_Error (*FT_BDF_GetPropertyFunc) (FT_Face face, const char *prop_name, BDF_PropertyRec * aproperty);


FT_DEFINE_SERVICE(BDF)
{
	FT_BDF_GetCharsetIdFunc get_charset_id;
	FT_BDF_GetPropertyFunc get_property;
};


FT_END_HEADER

#endif /* SVBDF_H_ */
