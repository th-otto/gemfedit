/***************************************************************************/
/*                                                                         */
/*  svgemfnt.h                                                             */
/*                                                                         */
/*    The FreeType GEM FNT service (specification).                        */
/*                                                                         */
/*  Copyright 2003 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __SVGEMFNT_H__
#define __SVGEMFNT_H__

#include <freetype/internal/ftserv.h>
#include <freetype/ftgemfnt.h>


FT_BEGIN_HEADER


#define FT_SERVICE_ID_GEMFNT  "gemfonts"

  typedef FT_Error
  (*FT_GemFnt_GetHeaderFunc)( FT_Face               face,
                              FT_GemFNT_HeaderRec  *aheader );


  FT_DEFINE_SERVICE( GemFnt )
  {
    FT_GemFnt_GetHeaderFunc  get_header;
  };

  /* */


FT_END_HEADER


#endif /* __SVGEMFNT_H__ */


/* END */
