/***************************************************************************/
/*                                                                         */
/*  ftgemfnt.c                                                             */
/*                                                                         */
/*    FreeType API for accessing GEM FNT specific info (body).             */
/*                                                                         */
/*  Copyright 2003, 2004 by                                                */
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
#include <freetype/ftgemfnt.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/internal/services/svgemfnt.h>

ANONYMOUS_STRUCT_DUMMY(FT_RasterRec_)
ANONYMOUS_STRUCT_DUMMY(FT_Size_InternalRec_)
ANONYMOUS_STRUCT_DUMMY(FT_IncrementalRec_)

  /* documentation is in ftgemfnt.h */

FT_EXPORT_DEF(FT_Error) FT_Get_GemFNT_Header(FT_Face face, FT_GemFNT_HeaderRec *header)
{
	FT_Service_GemFnt service;
	FT_Error error;

	error = FT_ERR(Invalid_Argument);

	if (face != NULL)
	{
		FT_FACE_LOOKUP_SERVICE(face, service, GEMFNT);

		if (service != NULL)
		{
			error = service->get_header(face, header);
		}
	}

	return error;
}
