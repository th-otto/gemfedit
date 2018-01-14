/***************************************************************************/
/*                                                                         */
/*  gemfnt.h                                                               */
/*                                                                         */
/*    FreeType font driver for GEM FNT files                               */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2007 by                         */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*  Copyright 2007 Dmitry Timoshkov for Codeweavers                        */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __GEMFNT_H__
#define __GEMFNT_H__


#include <ft2build.h>
#include <freetype/ftgemfnt.h>
#include <freetype/internal/ftdriver.h>


FT_BEGIN_HEADER

typedef struct FNT_FontRec_
{
	FT_ULong offset;

	FT_GemFNT_HeaderRec header;

	FT_UInt default_char;
	FT_Byte *fnt_data;
	
	FT_Int16 *horizontal_offsets;
	FT_UInt16 *offsets;
	
	FT_ULong fnt_size;
	FT_String *family_name;

} FNT_FontRec, *FNT_Font;


typedef struct FNT_FaceRec_
{
	FT_FaceRec root;
	FNT_Font font;

	FT_CharMap charmap_handle;
	FT_CharMapRec charmap;				/* a single charmap per face */

} FNT_FaceRec, *FNT_Face;


FT_EXPORT_VAR(const FT_Driver_ClassRec) gemfnt_driver_class;


FT_END_HEADER

#endif /* __GEMFNT_H__ */
