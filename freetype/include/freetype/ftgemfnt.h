/***************************************************************************/
/*                                                                         */
/*  ftgemfnt.h                                                             */
/*                                                                         */
/*    FreeType API for accessing GEM fnt-specific data.                    */
/*                                                                         */
/*  Copyright 2003, 2004, 2008 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTGEMFNT_H__
#define __FTGEMFNT_H__

#include <ft2build.h>
#include <freetype/freetype.h>

#ifdef FREETYPE_H
#error "freetype.h of FreeType 1 has been loaded!"
#error "Please fix the directory search order for header files"
#error "so that freetype.h of FreeType 2 is found first."
#endif


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    gemfnt_fonts                                                       */
  /*                                                                       */
  /* <Title>                                                               */
  /*    GEM FNT Files                                                      */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    GEM FNT specific API.                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains the declaration of GEM FNT specific          */
  /*    functions.                                                         */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_GemFNT_HeaderRec                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    GEM FNT Header info.                                               */
  /*                                                                       */
  typedef struct  FT_GemFNT_HeaderRec_
  {
    FT_UInt16  font_id;
    FT_UInt16  nominal_point_size;
    FT_Byte    face_name[32];
    FT_UInt16  first_ade;
    FT_UInt16  last_ade;
    FT_UInt16  top;
    FT_UInt16  ascent;
    FT_UInt16  half;
    FT_UInt16  descent;
    FT_UInt16  bottom;
    FT_UInt16  max_char_width;
    FT_UInt16  max_cell_width;
    FT_Int16   left_offset;
    FT_Int16   right_offset;
    FT_UInt16  thicken;
    FT_UInt16  underline_size;
    FT_UInt16  lighten;
    FT_UInt16  skew;
    FT_UInt16  flags;
    FT_UInt32  horizontal_table_offset;
    FT_UInt32  offset_table_offset;
    FT_UInt32  data_table_offset;
    FT_UInt16  form_width;
    FT_UInt16  form_height;
    FT_UInt32  next_font;
    
  } FT_GemFNT_HeaderRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_GemFNT_Header                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an @FT_GemFNT_HeaderRec structure.                     */
  /*                                                                       */
  typedef struct FT_GemFNT_HeaderRec_*  FT_GemFNT_Header;


  /**********************************************************************
   *
   * @function:
   *    FT_Get_GemFNT_Header
   *
   * @description:
   *    Retrieve a GEM FNT font info header.
   *
   * @input:
   *    face    :: A handle to the input face.
   *
   * @output:
   *    aheader :: The GemFNT header.
   *
   * @return:
   *   FreeType error code.  0~means success.
   *
   * @note:
   *   This function only works with GEM FNT faces, returning an error
   *   otherwise.
   */
  FT_EXPORT( FT_Error )
  FT_Get_GemFNT_Header( FT_Face               face,
                        FT_GemFNT_HeaderRec  *aheader );


  /* */

FT_END_HEADER

#endif /* __FTGEMFNT_H__ */


/* END */


/* Local Variables: */
/* coding: utf-8    */
/* End:             */
