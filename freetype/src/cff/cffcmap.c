/***************************************************************************/
/*                                                                         */
/*  cffcmap.c                                                              */
/*                                                                         */
/*    CFF character mapping table (cmap) support (body).                   */
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
#include "cffcmap.h"
#include "cffload.h"

#include "cfferrs.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****           CFF STANDARD (AND EXPERT) ENCODING CMAPS            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_CALLBACK_DEF( FT_Error )
  cff_cmap_encoding_init( FT_CMap  cmap_,
                          FT_Pointer   pointer )
  {
    CFF_CMapStd  cmap = (CFF_CMapStd)cmap_;
    TT_Face       face     = (TT_Face)FT_CMAP_FACE( cmap );
    CFF_Font      cff      = (CFF_Font)face->extra.data;
    CFF_Encoding  encoding = &cff->encoding;

    FT_UNUSED( pointer );


    cmap->gids  = encoding->codes;

    return 0;
  }


  FT_CALLBACK_DEF( void )
  cff_cmap_encoding_done( FT_CMap  cmap_ )
  {
    CFF_CMapStd  cmap = (CFF_CMapStd)cmap_;
    cmap->gids  = NULL;
  }


  FT_CALLBACK_DEF( FT_UInt32 )
  cff_cmap_encoding_char_index( FT_CMap cmap_,
                                FT_UInt32    char_code )
  {
    CFF_CMapStd  cmap = (CFF_CMapStd)cmap_;
    FT_UInt  result = 0;


    if ( char_code < 256 )
      result = cmap->gids[char_code];

    return result;
  }


  FT_CALLBACK_DEF( FT_UInt32 )
  cff_cmap_encoding_char_next( FT_CMap   cmap_,
                               FT_UInt32    *pchar_code )
  {
    CFF_CMapStd  cmap = (CFF_CMapStd)cmap_;
    FT_UInt    result    = 0;
    FT_UInt32  char_code = *pchar_code;


    *pchar_code = 0;

    if ( char_code < 255 )
    {
      FT_UInt  code = (FT_UInt)(char_code + 1);


      for (;;)
      {
        if ( code >= 256 )
          break;

        result = cmap->gids[code];
        if ( result != 0 )
        {
          *pchar_code = code;
          break;
        }

        code++;
      }
    }
    return result;
  }


  FT_DEFINE_CMAP_CLASS(
    cff_cmap_encoding_class_rec,

    sizeof ( CFF_CMapStdRec ),

    cff_cmap_encoding_init,        /* FT_CMap_InitFunc init       */
    cff_cmap_encoding_done,        /* FT_CMap_DoneFunc done       */
    cff_cmap_encoding_char_index,  /* FT_CMap_CharIndexFunc char_index */
    cff_cmap_encoding_char_next,   /* FT_CMap_CharNextFunc char_next  */

    NULL,  /* FT_CMap_CharVarIndexFunc char_var_index   */
    NULL,  /* FT_CMap_CharVarIsDefaultFunc char_var_default */
    NULL,  /* FT_CMap_VariantListFunc variant_list     */
    NULL,  /* FT_CMap_CharVariantListFunc charvariant_list */
    NULL   /* FT_CMap_VariantCharListFunc variantchar_list */
  )


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              CFF SYNTHETIC UNICODE ENCODING CMAP              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_CALLBACK_DEF( const char* )
  cff_sid_to_glyph_name( FT_Pointer data,
                         FT_UInt32 idx )
  {
    TT_Face face = data;
    CFF_Font     cff     = (CFF_Font)face->extra.data;
    CFF_Charset  charset = &cff->charset;
    FT_UInt      sid     = charset->sids[idx];


    return cff_index_get_sid_string( cff, sid );
  }


  FT_CALLBACK_DEF( FT_Error )
  cff_cmap_unicode_init( FT_CMap cmap_,
                         FT_Pointer   pointer )
  {
    PS_Unicodes  unicodes = (PS_Unicodes)cmap_;
    TT_Face             face    = (TT_Face)FT_CMAP_FACE( unicodes );
    FT_Memory           memory  = FT_FACE_MEMORY( face );
    CFF_Font            cff     = (CFF_Font)face->extra.data;
    CFF_Charset         charset = &cff->charset;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;

    FT_UNUSED( pointer );


    /* can't build Unicode map for CID-keyed font */
    /* because we don't know glyph names.         */
    if ( !charset->sids )
      return FT_THROW( No_Unicode_Glyph_Name );

    return psnames->unicodes_init( memory,
                                   unicodes,
                                   cff->num_glyphs,
                                   cff_sid_to_glyph_name,
                                   (PS_FreeGlyphNameFunc)NULL,
                                   (FT_Pointer)face );
  }


  FT_CALLBACK_DEF( void )
  cff_cmap_unicode_done( FT_CMap cmap_ )
  {
    PS_Unicodes  unicodes = (PS_Unicodes)cmap_;
    FT_Face    face   = FT_CMAP_FACE( unicodes );
    FT_Memory  memory = FT_FACE_MEMORY( face );


    FT_FREE( unicodes->maps );
    unicodes->num_maps = 0;
  }


  FT_CALLBACK_DEF( FT_UInt32 )
  cff_cmap_unicode_char_index( FT_CMap cmap_,
                               FT_UInt32    char_code )
  {
    PS_Unicodes  unicodes = (PS_Unicodes)cmap_;
    TT_Face             face    = (TT_Face)FT_CMAP_FACE( unicodes );
    CFF_Font            cff     = (CFF_Font)face->extra.data;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


    return psnames->unicodes_char_index( unicodes, char_code );
  }


  FT_CALLBACK_DEF( FT_UInt32 )
  cff_cmap_unicode_char_next( FT_CMap cmap_,
                              FT_UInt32   *pchar_code )
  {
    PS_Unicodes  unicodes = (PS_Unicodes)cmap_;
    TT_Face             face    = (TT_Face)FT_CMAP_FACE( unicodes );
    CFF_Font            cff     = (CFF_Font)face->extra.data;
    FT_Service_PsCMaps  psnames = (FT_Service_PsCMaps)cff->psnames;


    return psnames->unicodes_char_next( unicodes, pchar_code );
  }


  FT_DEFINE_CMAP_CLASS(
    cff_cmap_unicode_class_rec,

    sizeof ( PS_UnicodesRec ),

    cff_cmap_unicode_init,        /* FT_CMap_InitFunc init       */
    cff_cmap_unicode_done,        /* FT_CMap_DoneFunc done       */
    cff_cmap_unicode_char_index,  /* FT_CMap_CharIndexFunc char_index */
    cff_cmap_unicode_char_next,   /* FT_CMap_CharNextFunc char_next  */

    NULL,  /* FT_CMap_CharVarIndexFunc char_var_index   */
    NULL,  /* FT_CMap_CharVarIsDefaultFunc char_var_default */
    NULL,  /* FT_CMap_VariantListFunc variant_list     */
    NULL,  /* FT_CMap_CharVariantListFunc charvariant_list */
    NULL   /* FT_CMap_VariantCharListFunc variantchar_list */
  )


/* END */
