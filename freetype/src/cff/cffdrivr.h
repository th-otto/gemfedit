/***************************************************************************/
/*                                                                         */
/*  cffdrivr.h                                                             */
/*                                                                         */
/*    High-level OpenType driver interface (specification).                */
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


#ifndef CFFDRIVER_H_
#define CFFDRIVER_H_


#include <ft2build.h>
#include <freetype/internal/ftdriver.h>


FT_BEGIN_HEADER

FT_CALLBACK_TABLE const FT_Driver_ClassRec cff_driver_class;

#ifdef FT_CONFIG_OPTION_PIC
FT_Error FT_Create_Class_cff_services(FT_Library library, FT_ServiceDescRec ** output_class);
void FT_Destroy_Class_cff_services(FT_Library library, FT_ServiceDescRec * clazz);
void FT_Init_Class_cff_service_ps_info(FT_Library library, FT_Service_PsInfoRec * clazz);
void FT_Init_Class_cff_service_glyph_dict(FT_Library library, FT_Service_GlyphDictRec * clazz);
void FT_Init_Class_cff_service_ps_name(FT_Library library, FT_Service_PsFontNameRec * clazz);
void FT_Init_Class_cff_service_get_cmap_info(FT_Library library, FT_Service_TTCMapsRec * clazz);
void FT_Init_Class_cff_service_cid_info(FT_Library library, FT_Service_CIDRec * clazz);
#endif

FT_END_HEADER

#endif /* CFFDRIVER_H_ */
