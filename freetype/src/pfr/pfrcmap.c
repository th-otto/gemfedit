/***************************************************************************/
/*                                                                         */
/*  pfrcmap.c                                                              */
/*                                                                         */
/*    FreeType PFR cmap handling (body).                                   */
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
#include <freetype/internal/ftdebug.h>
#include "pfrcmap.h"
#include "pfrobjs.h"

#include "pfrerror.h"

ANONYMOUS_STRUCT_DUMMY(FT_RasterRec_)
ANONYMOUS_STRUCT_DUMMY(FT_IncrementalRec_)

FT_CALLBACK_DEF(FT_Error) pfr_cmap_init(FT_CMap cmap_, FT_Pointer pointer)
{
	PFR_CMap cmap = (PFR_CMap) cmap_;
	FT_Error error = FT_Err_Ok;
	PFR_Face face = (PFR_Face) FT_CMAP_FACE(cmap);

	FT_UNUSED(pointer);

	cmap->num_chars = face->phy_font.num_chars;
	cmap->chars = face->phy_font.chars;

	/* just for safety, check that the character entries are correctly */
	/* sorted in increasing character code order                       */
	{
		FT_UInt n;

		for (n = 1; n < cmap->num_chars; n++)
		{
			if (cmap->chars[n - 1].char_code >= cmap->chars[n].char_code)
			{
				error = FT_THROW(Invalid_Table);
				goto Exit;
			}
		}
	}

  Exit:
	return error;
}


FT_CALLBACK_DEF(void) pfr_cmap_done(FT_CMap cmap_)
{
	PFR_CMap cmap = (PFR_CMap) cmap_;

	cmap->chars = NULL;
	cmap->num_chars = 0;
}


FT_CALLBACK_DEF(FT_UInt32) pfr_cmap_char_index(FT_CMap cmap_, FT_UInt32 char_code)
{
	PFR_CMap cmap = (PFR_CMap) cmap_;
	FT_UInt min = 0;
	FT_UInt max = cmap->num_chars;

	while (min < max)
	{
		PFR_Char gchar;
		FT_UInt mid;

		mid = min + (max - min) / 2;
		gchar = cmap->chars + mid;

		if (gchar->char_code == char_code)
			return mid + 1;

		if (gchar->char_code < char_code)
			min = mid + 1;
		else
			max = mid;
	}
	return 0;
}


FT_CALLBACK_DEF(FT_UInt32) pfr_cmap_char_next(FT_CMap cmap_, FT_UInt32 * pchar_code)
{
	PFR_CMap cmap = (PFR_CMap) cmap_;
	FT_UInt result = 0;
	FT_UInt32 char_code = *pchar_code + 1;

  Restart:
	{
		FT_UInt min = 0;
		FT_UInt max = cmap->num_chars;
		FT_UInt mid;
		PFR_Char gchar;

		while (min < max)
		{
			mid = min + ((max - min) >> 1);
			gchar = cmap->chars + mid;

			if (gchar->char_code == char_code)
			{
				result = mid;
				if (result != 0)
				{
					result++;
					goto Exit;
				}

				char_code++;
				goto Restart;
			}

			if (gchar->char_code < char_code)
				min = mid + 1;
			else
				max = mid;
		}

		/* we didn't find it, but we have a pair just above it */
		char_code = 0;

		if (min < cmap->num_chars)
		{
			gchar = cmap->chars + min;
			result = min;
			if (result != 0)
			{
				result++;
				char_code = gchar->char_code;
			}
		}
	}

  Exit:
	*pchar_code = char_code;
	return result;
}


FT_CALLBACK_TABLE_DEF const FT_CMap_ClassRec pfr_cmap_class_rec = {
	sizeof(PFR_CMapRec),

	pfr_cmap_init,						/* FT_CMap_InitFunc init       */
	pfr_cmap_done,						/* FT_CMap_DoneFunc done       */
	pfr_cmap_char_index,				/* FT_CMap_CharIndexFunc char_index */
	pfr_cmap_char_next,					/* FT_CMap_CharNextFunc char_next  */

	NULL,								/* FT_CMap_CharVarIndexFunc char_var_index   */
	NULL,								/* FT_CMap_CharVarIsDefaultFunc char_var_default */
	NULL,								/* FT_CMap_VariantListFunc variant_list     */
	NULL,								/* FT_CMap_CharVariantListFunc charvariant_list */
	NULL								/* FT_CMap_VariantCharListFunc variantchar_list */
};
